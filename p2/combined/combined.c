#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include "combined.h"
#include "sense_reversal_extended.h"
#include "mcs_tree.h"

static int debug_level = 1;

static int threads_max;
static int process_max;
static int process_id;

void combined_init(int num_threads, int num_processes, int rank){
    if(debug_level >= 1) 
        printf("[COMBINED %d] INIT.\n", rank);

    threads_max = num_threads;
    process_max = num_processes;
    process_id = rank;

    sense_reversal_extended_init(threads_max);
    mcstree_init(process_max, process_id);
}

void combined_barrier(int thread_id){
    if(debug_level >= 1) 
        printf("[COMBINED %d-%d] stared.\n", process_id, thread_id);

    sense_reversal_extended_barrier(process_id, thread_id);
    
    if(debug_level >= 1) 
        printf("[COMBINED %d-%d] completed.\n", process_id, thread_id);
}

void combined_finalize(){
    if(debug_level >= 1) 
        printf("[COMBINED %d] started.\n", process_id);

    mcstree_finalize();
    sense_reversal_extended_finalize();
}

