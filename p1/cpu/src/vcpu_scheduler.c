#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include<stdbool.h>
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)
#define DOMAIN_COUNT_MAX 16
#define PCPU_COUNT_MAX 16

int is_exit = 0; // DO NOT MODIFY THIS VARIABLE

typedef unsigned long long ll;

bool is_first = true;
int debug_level = 1;

int domain_count;
int pcpu_count;
int readability_factor = 10;

// Assume that we have one vCPU per VM
struct DomainVCPUStats{
	ll previoustime;
	ll currenttime;
	ll usage;
} host_cpu_stats[PCPU_COUNT_MAX], domain_vcpu_stats[DOMAIN_COUNT_MAX];

struct HostCPUMapping{
	int pcpu;
	int count;
	int mapping[DOMAIN_COUNT_MAX];
} host_mapping[PCPU_COUNT_MAX];

virDomainPtr* domains;

/**
 * Gets all the active domains.
 * Runs only once as the number of vms never change.
 * - Updates the domain pointers to point to correct domains.
 * - Also updates the number of domains running.
 * - Sets is_first to false.
 **/
void getActiveDomains(virConnectPtr conn){
	if(!is_first)
		return;
	
	domain_count = virConnectListAllDomains(conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
	pcpu_count = virNodeGetCPUMap(conn, NULL, NULL, 0);

	if(debug_level >= 1) 
		printf("found %d domains on %d pcpus.\n", domain_count, pcpu_count);
}

void getHostCPUStat(virConnectPtr conn, int pcpu){
	virNodeCPUStatsPtr node_cpu_stats;
	int params_size = 0;
	
	virNodeGetCPUStats(conn, 0, NULL, &params_size, 0);

	if (params_size >= 0){
		if(debug_level >= 2)
			printf("retrieved %d parameters.\n", params_size);
		
		if(params_size != 0){
			node_cpu_stats = malloc(sizeof(virNodeCPUStats)*params_size);
			virNodeGetCPUStats(conn, pcpu, node_cpu_stats, &params_size, 0);

			for (int i = 0; i < params_size; i++) {
				if (!strcmp(VIR_NODE_CPU_STATS_USER, node_cpu_stats[i].field) ||
		    		!strcmp(VIR_NODE_CPU_STATS_KERNEL, node_cpu_stats[i].field)){
					host_cpu_stats[pcpu].previoustime = host_cpu_stats[pcpu].currenttime;
					host_cpu_stats[pcpu].currenttime = node_cpu_stats[i].value;
					host_cpu_stats[pcpu].usage = host_cpu_stats[pcpu].currenttime - host_cpu_stats[pcpu].previoustime;
				}
			}

			free(node_cpu_stats);			
		}
		else{
			if(debug_level >= 2)
				printf("params_size is 0.\n");
			return;
		}
	}
	else{
		if(debug_level >= 2)
			printf("failed to set params_size.\n");
		return;
	}
}

void getHostCPUStats(virConnectPtr conn){
	for(int i = 0; i < pcpu_count; i++)
		getHostCPUStat(conn, i);
}

void cleanHostCPUStats(){
	for(int i = 0; i < pcpu_count; i++){
		host_cpu_stats[i].usage = 0;

		host_mapping[i].pcpu = i;
		host_mapping[i].count = 0;
	}
}

void getDomainCPUStat(int index){
	virVcpuInfoPtr node_cpu_stats;
	int params_size = virDomainGetCPUStats(domains[index], NULL, 0, 0, 1, 0);

	if (params_size >= 0){
		if(debug_level >= 2)
			printf("retrieved %d parameters.\n", params_size);
		
		if(params_size != 0){
			node_cpu_stats = malloc(sizeof(virVcpuInfoPtr));
			virDomainGetVcpus(domains[index], node_cpu_stats, 1, NULL, 0);

			domain_vcpu_stats[index].previoustime = domain_vcpu_stats[index].currenttime;
			domain_vcpu_stats[index].currenttime = node_cpu_stats->cpuTime;
			domain_vcpu_stats[index].usage = domain_vcpu_stats[index].currenttime - domain_vcpu_stats[index].previoustime;

			host_cpu_stats[node_cpu_stats->cpu].usage += domain_vcpu_stats[index].usage;

			host_mapping[node_cpu_stats->cpu].mapping[host_mapping[node_cpu_stats->cpu].count] = index;
			host_mapping[node_cpu_stats->cpu].count++;

			if(debug_level >= 2)
				printf("vcpu %d -> pcpu %d\n", index, node_cpu_stats->cpu);

			free(node_cpu_stats);
		}
		else{
			if(debug_level >= 2)
				printf("params_size is 0.\n");
			return;
		}
	}
	else{
		if(debug_level >= 2)
			printf("failed to set params_size.\n");
		return;
	}
}

/**
 * Gets the stats for all the domains in the domains list.
 **/ 
void getDomainCPUStats(){
	for(int i = 0; i < domain_count; i++)
		getDomainCPUStat(i);
}

/**
 * Simply prints all the required stats required for understanding the algorithm.
 **/
void printMemoryStats(){
	for(int i = 0; i < pcpu_count; i++){
		printf("host  %d: ", i);
		printf("cputime - [%5llu] ",  host_cpu_stats[i].usage>>readability_factor);
		
		printf("mappings [");
		for(int j = 0; j < host_mapping[i].count; j++)
			printf("%3d", host_mapping[i].mapping[j]);
		printf("]\n");
	}

	for(int i = 0; i < domain_count; i++){
		printf("%s : ", virDomainGetName(domains[i]));
		printf("cputime - [%5llu] ",  domain_vcpu_stats[i].usage>>readability_factor);
		printf("\n");
	}
}

/**
 * Gets the CPU stats
 **/
void getCPUStats(virConnectPtr conn){
	// Do not use this function as this will add up the time from the base host machine.
	// The Domain CPU stats function updates the host by adding the underlying pcpu.
	// getHostCPUStats(conn);
	cleanHostCPUStats();
	getDomainCPUStats();
}

void CPUScheduler(virConnectPtr conn,int interval){
	printf("-------------------------------------------------\n");
	getActiveDomains(conn);
	getCPUStats(conn);

	if(debug_level >= 1) 
		printMemoryStats();

	if(is_first) is_first = false;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler() {
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[]) {
	virConnectPtr conn;

	if(argc != 2) {
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);
	
	conn = virConnectOpen("qemu:///system");
	if(conn == NULL) {
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	// Get the total number of pCpus in the host
	signal(SIGINT, signal_callback_handler);

	while(!is_exit) // Run the CpuScheduler function that checks the CPU Usage and sets the pin at an interval of "interval" seconds
	{
		CPUScheduler(conn, interval);
		sleep(interval);
	}

	// Closing the connection
	virConnectClose(conn);
	return 0;
}
