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

int is_exit = 0; // DO NOT MODIFY THE VARIABLE

typedef long ll;

bool is_first = true;
int debug_level = 1;

int domain_count = 4;
int readability_factor = 10;

// Algorithm data
const int domain_available_min = 200;
const int domain_usable_min = 100;
const int domain_diff_max = 1>>5;
const int domain_diff_min = 1>>2;
const float variance_threshold = 0.1;
const float domain_usable_min_low = domain_usable_min*(1-variance_threshold);
const float domain_usable_min_high = domain_usable_min*(1+variance_threshold);

/**
 * 1 = domain_usable_min buffer constant
 * 2 = maximum of domain_usable_min or 10% of used
 **/ 
const int buffer_policy = 1;

ll host_distance, distance[DOMAIN_COUNT_MAX], changed[DOMAIN_COUNT_MAX];

struct DomainMemoryStats{
	ll envisioned; // Total max the host can get for the domain.
	ll committed; // Total available to machine os from host's perspective.
	ll available; // same as committed, but source is different api.
	ll usable; // How much the balloon can be extended.
	ll unused; 
	ll ballooned; // ??? Baloon size max. https://github.com/collectd/collectd/issues/2237
} host_memory_stats, domain_memory_stats[DOMAIN_COUNT_MAX];

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

	if(debug_level >= 1) 
		printf("found %d domains.\n", domain_count);
}

/**
 * Retreives memory information for a domain.
 * - Updates the domain_memory_stat location for the given domain with index.
 **/
void getDomainMemoryStat(int index){
	virDomainInfo vir_domain_info;
	virDomainGetInfo(domains[index], &vir_domain_info);

	domain_memory_stats[index].envisioned = vir_domain_info.maxMem;
	domain_memory_stats[index].committed = vir_domain_info.memory;
	
	virDomainMemoryStatPtr domain_memory_stat_raw = 
			malloc(sizeof(virDomainMemoryStatStruct)*VIR_DOMAIN_MEMORY_STAT_NR);
	virDomainMemoryStats(domains[index], domain_memory_stat_raw, VIR_DOMAIN_MEMORY_STAT_NR, 0);

	for(int i = 0; i < VIR_DOMAIN_MEMORY_STAT_NR; i++){
		ll value = domain_memory_stat_raw[i].val;

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
	free(domain_memory_stat_raw);
}

/**
 * Gets the stats for all the domains in the domains list.
 **/ 
void getDomainMemoryStats(){
	for(int i = 0; i < domain_count; i++)
		getDomainMemoryStat(i);
}

/**
 * Simply prints all the required stats required for understanding the algorithm.
 **/
void printMemoryStats(){
	printf("host    : ");
	printf("available - [%5ld] ",  host_memory_stats.available>>readability_factor);
	printf("unused - [%5ld] ",  host_memory_stats.unused>>readability_factor);
	printf("\n");

	for(int i = 0; i < domain_count; i++){
		printf("%s : ", virDomainGetName(domains[i]));
		printf("committed - [%5ld] ",  domain_memory_stats[i].committed>>readability_factor);
		printf("unused - [%5ld] ",  domain_memory_stats[i].unused>>readability_factor);
		printf("distance - [%5ld] ", distance[i]>>readability_factor);
		printf("changed - [%5ld] ", changed[i]>>readability_factor);
		printf("\n");
	}
}

/**
 * Gets all memory stats for the host.
 **/
void getHostMemoryStats(virConnectPtr conn){
	virNodeMemoryStatsPtr node_memory_stats;
	int params_size = 0;

	if (!virNodeGetMemoryStats(conn, VIR_NODE_MEMORY_STATS_ALL_CELLS, NULL, &params_size, 0)){
		if(debug_level >= 2)
			printf("retrieved %d parameters.\n", params_size);
		
		if(params_size != 0){
			node_memory_stats = malloc(sizeof(virNodeMemoryStats) * params_size);
			virNodeGetMemoryStats(conn, VIR_NODE_MEMORY_STATS_ALL_CELLS, node_memory_stats, &params_size, 0);

			for(int i = 0; i < params_size; i++){
				if(debug_level >= 2)
					printf("%s %llu\n", node_memory_stats->field, node_memory_stats->value);

				ll value = node_memory_stats->value;

				if(!strcmp(VIR_NODE_MEMORY_STATS_TOTAL, node_memory_stats->field))
					host_memory_stats.available = value;
				if(!strcmp(VIR_NODE_MEMORY_STATS_FREE, node_memory_stats->field))
					host_memory_stats.usable = value;
			}

			host_memory_stats.unused = virNodeGetFreeMemory(conn)>>10;
			free(node_memory_stats);
		}
		else{
			if(debug_level >= 2)
				printf("params_size is 0.\n");
		}
	}
	else{
		if(debug_level >= 2)
			printf("failed to set params_size.\n");
	}
}

/**
 * Gets all memory stats required for the scheduler.
 **/
void getMemoryStats(virConnectPtr conn){
	getHostMemoryStats(conn);
	getDomainMemoryStats();
}

/**
 * Get the distance from the buffer based on the buffer policy.
 **/ 
int getDistance(struct DomainMemoryStats domain_memory_stat){
	int unused = domain_memory_stat.unused;
	int committed = domain_memory_stat.committed;

	switch (buffer_policy){
		case 1:
			return unused - domain_available_min;		
		case 2:
			return unused - MAX(domain_usable_min, committed/10);
		default:
			return unused - domain_available_min;
	}
}

/**
 * Populate the distances to the normal buffer size.
 **/
void populateDistance(){
	host_distance = getDistance(host_memory_stats);

	for(int i = 0; i < domain_count; i++)
		distance[i] = getDistance(domain_memory_stats[i]);
}

bool getFeasibilityAndPopulateChanges(int interval){	
	ll effective_host_distance = host_distance;

	for(int i = 0; i < domain_count; i++){
		if(distance[i] < domain_usable_min_low)
			changed[i] = domain_usable_min*inteval;
		else if(distance[i] > domain_usable_min_high){
			if(distance[i] < 4*domain_usable_min)
				changed[i] = -int(interval*(domain_usable_min_max - domain_usable_min_low));
			else
				changed[i] = -distance[i]>>2;
		}
		else
			changed[i] = 0;

		effective_host_distance -= changed[i];
	}
	
	return effective_host_distance > 2*(domain_usable_min_high - domain_usable_min); 
}

void balanceChanges(){
	ll to_give =  
}

/**
 * This actually does the memory call to increase memory fo domain to value.
 **/  
void changeMemory(int domain, ll value){
		
}

/**
 * Execute the changes. 
 * First reclaim memory from hosts and then give out excess.
 **/ 
void executeChanges(){
	for(int i = 0; i < domain_count; i++){
		if(changed[i] <= 0){
			if(debug_level >= 1)
				printf("Executing %d with diff %d from %d to %d.\n", 
					i, changed[i], domain_memory_stats[i].committed,
					domain_memory_stats[i].committed + changed[i]
					);
			changeMemory(i, domain_memory_stats[i].committed + changed[i]);
		}
	}

	for(int i = 0; i < domain_count; i++){
		if(changed[i] > 0){
			printf("Executing %d with diff %d from %d to %d.\n", 
					i, changed[i], domain_memory_stats[i].committed,
					domain_memory_stats[i].committed + changed[i]
					);
			changeMemory(i, domain_memory_stats[i].committed + changed[i]);
		}
	}
}

/**
 * The memory scheduler algorithm.
 **/
void MemoryScheduler(virConnectPtr conn, int interval){
	printf("-------------------------------------------------\n");
	getActiveDomains(conn);
	getMemoryStats(conn);
	
	populateDistance();

	bool feasible = getFeasibilityAndPopulateChanges(interval);
	if(!feasible)
		balanceChanges();
	executeChanges();
	
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
int main(int argc, char *argv[]){
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

	signal(SIGINT, signal_callback_handler);

	while(!is_exit) {
		// Calls the MemoryScheduler function after every 'interval' seconds
		MemoryScheduler(conn, interval);
		sleep(interval);
	}

	// Close the connection
	virConnectClose(conn);
	return 0;
}
