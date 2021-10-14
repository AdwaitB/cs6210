#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include "sense_reversal_extended.h"
#include "mcs_tree.h"

static bool sense;
static int count;
static int count_max;

static int debug_level = 1;

void sense_reversal_extended_init(int num_threads){
    count_max = num_threads;
    sense = 0;
    count = count_max;
}

void sense_reversal_extended_barrier(int thread_id){
    if(debug_level >= 1) 
        printf("[PROG %d] stared.\n", thread_id);

    bool old = sense;
    
    if(__sync_fetch_and_sub(&count, 1) == 1){
        if(debug_level >= 1) 
            printf("[PROG %d] last.\n", thread_id);
        count = count_max;

        // mcs extention
        mcstree_barrier();

        sense = !old;
    }
    else{
        if(debug_level >= 1) 
            printf("[PROG %d] not last.\n", thread_id);
        while(sense == old);
    }
    
    if(debug_level >= 1) 
        printf("[PROG %d] completed.\n", thread_id);
}

void sense_reversal_extended_finalize(){

}

