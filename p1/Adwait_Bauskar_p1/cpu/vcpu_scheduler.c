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

float threshold = 0.05;

// Assume that we have one vCPU per VM
struct DomainVCPUStats{
	ll previoustime;
	ll currenttime;
	ll usage;
} domain_vcpu_stats[DOMAIN_COUNT_MAX];

bool domain_tagged[DOMAIN_COUNT_MAX];

struct HostCPUMapping{
	ll usage;
	int count;
	int mapping[DOMAIN_COUNT_MAX];
} host_mapping[PCPU_COUNT_MAX], host_mapping_next[PCPU_COUNT_MAX];

int leavage_individual_current = 2;

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

/**

 * This includes a lot of other values from the host.
 * Not using thi as of now as we do not know which time amoung this was used by the domain.

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
**/

/**
 * This cleans the host cpu stats.
 * We calculate the host cpu stats by adding the usages on the domains and the associated mapping.
 **/ 
void cleanCPUStats(){
	for(int i = 0; i < pcpu_count; i++){
		host_mapping[i].usage = 0;
		host_mapping[i].count = 0;

		host_mapping_next[i].usage = 0;
		host_mapping_next[i].count = 0;
	}
}

/**
 * This gets the domain CPU stats.
 * Also gets the mapping between vcpu and pcpu.
 * Then updates the host stats by adding vcpu using the mapping.s
 **/
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

			host_mapping[node_cpu_stats->cpu].usage += domain_vcpu_stats[index].usage;

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
		printf("host  %d : ", i);
		printf("cputime - [%5llu] ",  host_mapping[i].usage>>readability_factor);
		
		printf("mappings [");
		for(int j = 0; j < host_mapping[i].count; j++)
			printf("%3d", host_mapping[i].mapping[j]);
		printf("]\n");
	}

	for(int i = 0; i < pcpu_count; i++){
		printf("host  %d : ", i);
		printf("cputime - [%5llu] ",  host_mapping_next[i].usage>>readability_factor);
		
		printf("mappings [");
		for(int j = 0; j < host_mapping_next[i].count; j++)
			printf("%3d", host_mapping_next[i].mapping[j]);
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
	cleanCPUStats();
	getDomainCPUStats();
}

int getUntaggedDomainWithLargestUsage(){
	int ret = -1;
	ll maxv = 0;
	
	for(int i = 0; i < domain_count; i++){
		if(!domain_tagged[i]){
			if((ret == -1) || (domain_vcpu_stats[i].usage > maxv)){
				ret = i;
				maxv = domain_vcpu_stats[i].usage;
			}
		}
	}

	return ret;
}

int getHostWithMinUsage(){
	int ret = 0;
	int mincount = host_mapping_next[0].count;
	ll minv = host_mapping_next[0].usage;

	for(int i = 0; i < pcpu_count; i++){
		if((host_mapping_next[i].usage < minv) || 
			((host_mapping_next[i].usage == minv) && (host_mapping_next[i].count < mincount))){
			ret = i;
			minv = host_mapping_next[i].usage;
			mincount = host_mapping_next[i].count;
		}
	}

	return ret;
}

/**
 * Updates the next mapping based on best fit.
 **/
void updateNextMapping(){
	for(int i = 0; i < domain_count; i++)
		domain_tagged[i] = false;

	for(int i = 0; i < domain_count; i++){
		int next_domain = getUntaggedDomainWithLargestUsage();
		domain_tagged[next_domain] = true;

		int next_host = getHostWithMinUsage();
		host_mapping_next[next_host].usage += domain_vcpu_stats[next_domain].usage;
		host_mapping_next[next_host].mapping[host_mapping_next[next_host].count] = next_domain;
		host_mapping_next[next_host].count++;
	}	
}

int compareInt(const void* a, const void* b){
    return (*((int*)a)) - (*((int*)b));
}

void calculateHostMappings(ll* old_usages, ll* new_usages){
	for(int i = 0; i < pcpu_count; i++){
		old_usages[i] = host_mapping[i].usage;
		new_usages[i] = host_mapping_next[i].usage;
	}

	qsort(old_usages, pcpu_count, sizeof(ll), compareInt);
	qsort(new_usages, pcpu_count, sizeof(ll), compareInt);

	if(debug_level >= 1){
		printf("usages  :\n");
		for(int i = 0; i < pcpu_count; i++)
			printf(" %llu", old_usages[i]>>readability_factor);
		printf("\n");
		for(int i = 0; i < pcpu_count; i++)
			printf(" %llu", new_usages[i]>>readability_factor);
		printf("\n");
	}
}

bool checkIndividualBoundaries(ll* old_usages, ll* new_usages){
	for(int i = 0; i < pcpu_count; i++){
		if((old_usages[i]*(1.0 - threshold) > new_usages[i]*1.0) || 
			(old_usages[i]*(1.0 + threshold) < new_usages[i]*1.0))
			return true;
	}

	return false;
}

bool checkThresholds(){
	ll old_usages[PCPU_COUNT_MAX], new_usages[PCPU_COUNT_MAX];
	calculateHostMappings(old_usages, new_usages);

	bool individual = checkIndividualBoundaries(old_usages, new_usages);

	if(individual) leavage_individual_current--;
	if(leavage_individual_current <= 0){
		individual = false;
		leavage_individual_current = 3;
	}

	return individual;
}

int getHostForDomain(int domain_id){
	for(int i = 0; i < pcpu_count; i++){
		for(int j = 0; j < host_mapping_next[i].count; j++){
			if(host_mapping_next[i].mapping[j] == domain_id)
				return i;
		}
	}
	return 0;
}

void changePinnings(){
	//return;
	printf("Changing pinnings.\n");

	for(int i = 0; i < domain_count; i++){
		// assume exactly one vcpu per vm
		int maplen = VIR_CPU_MAPLEN(pcpu_count);

		unsigned char* map = calloc(maplen, sizeof(char));
		int pcpu_value = getHostForDomain(i);
		*(map + (pcpu_value/8)) = 0x1<<(pcpu_value%8);

		int ret = virDomainPinVcpu(domains[i], 0, map, maplen);

		if(debug_level >= 1)
			printf("Mapping %d vcpu to %d pcpu %d.\n", i, getHostForDomain(i), ret);

		free(map);
	}
}

void CPUScheduler(virConnectPtr conn,int interval){
	printf("-------------------------------------------------\n");
	getActiveDomains(conn);
	getCPUStats(conn);

	updateNextMapping();

	if(debug_level >= 1) 
		printMemoryStats();

	if(checkThresholds() && !is_first) 
		changePinnings();

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
