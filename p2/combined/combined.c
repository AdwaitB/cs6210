#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include "combined.h"

static int debug_level = 1;

static int threads_max;
static int process_max;
static int process_id;

void combined_init(int num_threads, int num_processes, int rank){
    if(debug_level >= 1) 
        printf("[INIT %d] started.\n", rank);

    threads_max = num_threads;
    process_max = num_processes;
    process_id = rank;
}

void combined_barrier(int thread_id){
    if(debug_level >= 1) 
        printf("[PROG %d] stared.\n", thread_id);


    
    if(debug_level >= 1) 
        printf("[PROG %d] completed.\n", thread_id);
}

void combined_finalize(){
    if(debug_level >= 1) 
        printf("[INIT %d] started.\n", process_id);
}

