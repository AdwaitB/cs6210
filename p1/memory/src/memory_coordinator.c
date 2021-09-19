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
#define DOMAIN_COUNT_MAX 8

int is_exit = 0; // DO NOT MODIFY THE VARIABLE

typedef unsigned long lu;

bool is_first = true;
bool debug = true;

int domain_count = 4;
int readability_factor = 10;

struct DomainMemoryStats{
	lu envisioned; // Total max the host can get for the domain.
	lu committed; // Total available to machine os from host's perspective.
	lu available; // same as committed, but source is different api.
	lu usable; // How much the balloon can be extended.
	lu unused; 
	lu ballooned; // ??? Baloon size max. https://github.com/collectd/collectd/issues/2237
} host_memory_stats, domain_memory_stats[DOMAIN_COUNT_MAX];

virDomainPtr* domains;
char* domain_names[DOMAIN_COUNT_MAX];

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

	if(debug) printf("found %d domains.\n", domain_count);

	// for(int i = 0; i < domain_count; i++)
		// domain_names[i] = virDomainGetHostname(domains[i], 0);

	is_first = false;
}

/**
 * Retreives memory information for a domain.
 * - Updates the domain_memory_stat location for the given domain with index.
 **/
void getDomainMemoryStat(int index){
	virDomainInfo vir_domain_info;
	virDomainGetInfo(domains[index], &vir_domain_info);

	domain_memory_stats[index].envisioned = vir_domain_info.maxMem>>readability_factor;
	domain_memory_stats[index].committed = vir_domain_info.memory>>readability_factor;
	
	virDomainMemoryStatPtr domain_memory_stat_raw = 
			malloc(sizeof(virDomainMemoryStatStruct)*VIR_DOMAIN_MEMORY_STAT_NR);
	virDomainMemoryStats(domains[index], domain_memory_stat_raw, VIR_DOMAIN_MEMORY_STAT_NR, 0);

	for(int i = 0; i < VIR_DOMAIN_MEMORY_STAT_NR; i++){
		lu value = domain_memory_stat_raw[i].val>>readability_factor;

		switch(domain_memory_stat_raw[i].tag){
			case VIR_DOMAIN_MEMORY_STAT_AVAILABLE:
				domain_memory_stats[index].available = value; 
				break;
			case VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON:
				domain_memory_stats[index].ballooned = value;
				break;
			case VIR_DOMAIN_MEMORY_STAT_USABLE:
				domain_memory_stats[index].usable = value;
				break;
			case VIR_DOMAIN_MEMORY_STAT_UNUSED:
				domain_memory_stats[index].unused = value;
				break;
			default:
				break;
		}
	}
}

/**
 * Gets the stats for all the domains in the domains list.
 **/ 
void getDomainMemoryStats(){
	for(int i = 0; i < domain_count; i++)
		getDomainMemoryStat(i);
	
	if(debug){
		for(int i = 0; i < domain_count; i++){
			printf("%s", domain_names[i]);
			// printf(", envisioned - %lu",  domain_memory_stats[i].envisioned);
			// printf(", committed - %lu",  domain_memory_stats[i].committed);
			printf(", available - %lu",  domain_memory_stats[i].available);
			printf(", usable - %lu",  domain_memory_stats[i].usable);
			printf(", unused - %lu",  domain_memory_stats[i].unused);
			// printf(", balooned - %lu",  domain_memory_stats[i].ballooned);
			printf("\n");
		}
	}
}

/**
 * Gets all memory stats for the host.
 **/
void getHostMemoryStats(){

}

/**
 * Gets all memory stats required for the scheduler.
 **/
void getMemoryStats(){
	getHostMemoryStats();
	getDomainMemoryStats();
}

/**
 * The memory scheduler algorithm.
 **/
void MemoryScheduler(virConnectPtr conn,int interval){
	printf("Interval %d -------------------------------------------------\n", interval);
	getActiveDomains(conn);
	getMemoryStats();
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler()
{
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[])
{
	virConnectPtr conn;

	if(argc != 2)
	{
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);
	
	conn = virConnectOpen("qemu:///system");
	if(conn == NULL)
	{
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	{
		// Calls the MemoryScheduler function after every 'interval' seconds
		MemoryScheduler(conn, interval);
		sleep(interval);
	}

	// Close the connection
	virConnectClose(conn);
	return 0;
}
