#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include<stdbool.h>
#define MB 1024
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)
#define DOMAIN_COUNT_MAX 16

int is_exit = 0; // DO NOT MODIFY THE VARIABLE

typedef long long ll;

bool is_first = true;
int debug_level = 1;

int domain_count = 4;
int readability_factor = 10;

// Algorithm Heuristics
const ll domain_available_min = 200*MB;
const ll domain_usable_min = 100*MB;
const float variance_threshold = 0.1;
const float domain_usable_min_low = domain_usable_min*(1-variance_threshold);
const float domain_usable_min_high = domain_usable_min*(1+variance_threshold);
const float delta = domain_usable_min_high - domain_usable_min_low;

/**
 * 1 = domain_usable_min buffer constant
 * 2 = maximum of domain_usable_min or 10% of used
 **/ 
const int buffer_policy = 1;

// Never let the host distance go negative.
// distance is measured distance from unused. So starving vms will have negative distance.
// changed is measured as solution. So starving vms will have positive distance.
ll host_distance, distance[DOMAIN_COUNT_MAX], changed[DOMAIN_COUNT_MAX];

struct DomainMemoryStats{
	// Total max the host can get for the domain.
	ll envisioned;
	// Total available to machine os from host's perspective.
	ll committed; 
	// same as committed, but source is different api.
	ll available; 
	// How much the balloon can be extended.
	ll usable; 
	ll unused; 
	// ??? Baloon size max. https://github.com/collectd/collectd/issues/2237
	ll ballooned; 
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
 * Set the sampling for the memory stats collection.
 **/
void setSampling(int interval){
	if(!is_first)
		return;
	
	for(int i = 0; i < domain_count; i++)
		virDomainSetMemoryStatsPeriod(domains[i], interval, VIR_DOMAIN_AFFECT_CURRENT);
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
	if(is_first){
		printf("S_b [%5lld] ", domain_usable_min>>readability_factor);
		printf("delta [%.2f] ", delta/(1<<10));
		printf("\n");
	}

	printf("host    : ");
	//printf("committed - [%5lld] ",  host_memory_stats.committed>>readability_factor);
	printf("available - [%5lld] ",  host_memory_stats.available>>readability_factor);
	printf("unused - [%5lld] ",  host_memory_stats.unused>>readability_factor);
	printf("distance - [%5lld] ",  host_distance>>readability_factor);
	printf("\n");

	for(int i = 0; i < domain_count; i++){
		printf("%s : ", virDomainGetName(domains[i]));
		printf("envisioned - [%5lld] ",  domain_memory_stats[i].envisioned>>readability_factor);
		printf("committed - [%5lld] ",  domain_memory_stats[i].committed>>readability_factor);
		printf("unused - [%5lld] ",  domain_memory_stats[i].unused>>readability_factor);
		printf("distance - [%5lld] ", distance[i]>>readability_factor);
		printf("changed - [%5lld] ", changed[i]>>readability_factor);
		printf("\n");
	}
}

/**
 * Gets all memory stats for the host.
 **/
void getHostMemoryStats(virConnectPtr conn){
	host_memory_stats.committed = virNodeGetFreeMemory(conn)>>10;

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
			return unused - domain_usable_min;		
		case 2:
			return unused - MAX(domain_usable_min, committed/10);
		default:
			return unused - domain_usable_min;
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

/**
 * Mark the changes to be make to give or takeback memory.
 **/
bool getFeasibilityAndPopulateChanges(int interval){	
	ll effective_host_distance = host_distance;

	for(int i = 0; i < domain_count; i++){
		if(distance[i] < -0.5*delta)
			// S_b MB per interval
			changed[i] = domain_usable_min*interval;
		else if(distance[i] > 0.5*delta){
			// Release slowly if the extra memory is not so much
			if(distance[i] < 4*domain_usable_min)
				changed[i] = -1*(int)(interval*delta);
			// Release 25% if extra memory is very large.
			else
				changed[i] = -1*(distance[i]>>2);
		}
		else
			changed[i] = 0;

		effective_host_distance -= changed[i];
	}
	
	// Use the same buffer for range here as this marks the extra boundary buffer for the host.
	return effective_host_distance > 2*(domain_usable_min_high - domain_usable_min); 
}

/**
 * When the host doesnt have effective free memory to give out. Just give out the remaming memory greedily.
 **/
void balanceChanges(){
	ll host_available = host_distance;

	for(int i = 0; i < domain_count; i++)
		host_available += (changed[i] > 0 ? changed[i] : 0);
		
	for(int i = 0; i < domain_count; i++){
		if(host_available <= 0)
			break;

		if(changed[i] < 0){
			if(host_available > (-1*changed[i])){
				host_available += changed[i];
			}
			else{
				changed[i] = -1*host_available;
				host_available = 0;
				break;
			}
		}
		if((changed[i] < 0) && (host_available > -1*changed[i]))
			host_available += changed[i];
		else if(changed[i] < 0)
			changed[i] = -1*host_available;
	}	
}

/**
 * This actually does the memory call to increase memory fo domain to value.
 **/  
void changeMemory(int domain, ll value){
	// Check if the domain can be shinked.
	if(value < domain_available_min + domain_usable_min)
		return;

	// Check if something is changing or not.
	if(value == domain_memory_stats[domain].committed)
		return;

	// Check if we are setting more than max
	if(value >= domain_memory_stats[domain].envisioned)
		return;
	
	if(debug_level >= 1)
		printf("Executing %d with diff %lld from %lld to %lld.\n", 
			domain, changed[domain]>>readability_factor, domain_memory_stats[domain].committed>>readability_factor,
			(domain_memory_stats[domain].committed + changed[domain])>>readability_factor
		);
	
	//return;
	virDomainSetMemory(domains[domain], value);
}

/**
 * Execute the changes. 
 * First reclaim memory from hosts and then give out excess.
 **/ 
void executeChanges(){
	for(int i = 0; i < domain_count; i++){
		if(changed[i] <= 0)
			changeMemory(i, domain_memory_stats[i].committed + changed[i]);
	}

	for(int i = 0; i < domain_count; i++){
		if(changed[i] > 0)
			changeMemory(i, domain_memory_stats[i].committed + changed[i]);
	}
}

/**
 * The memory scheduler algorithm.
 **/
void MemoryScheduler(virConnectPtr conn, int interval){
	printf("-------------------------------------------------\n");
	getActiveDomains(conn);
	setSampling(interval);
	getMemoryStats(conn);
	
	if(debug_level >= 1) 
		populateDistance();

	bool feasible = getFeasibilityAndPopulateChanges(interval);

	printMemoryStats();

	if(!feasible)
		balanceChanges();
	executeChanges();

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
