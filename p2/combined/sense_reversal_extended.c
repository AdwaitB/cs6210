#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include "sense_reversal_extended.h"

static bool sense;
static int count;
static int count_max;

static int debug_level = 1;

void sense_reversal_extended_init(int num_threads){
    count_max = num_threads;
    sense = 0;
    count = count_max;
}

// process id needed here only for debugging
void sense_reversal_extended_barrier(int process_id, int thread_id, void (*extension)()){
    if(debug_level >= 1) 
        printf("[SENSE %d-%d] stared.\n", process_id, thread_id);

    bool old = sense;
    
    if(__sync_fetch_and_sub(&count, 1) == 1){
        if(debug_level >= 1) 
            printf("[SENSE %d-%d] last.\n", process_id, thread_id);
        count = count_max;

        // extention
        extension();

        sense = !old;
    }
    else{
        if(debug_level >= 1) 
            printf("[SENSE %d-%d] not last.\n", process_id, thread_id);
        while(sense == old);
    }
    
    if(debug_level >= 1) 
        printf("[SENSE %d-%d] completed.\n", process_id, thread_id);
}

void sense_reversal_extended_finalize(){

}

