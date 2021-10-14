#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include "gtmp.h"

static bool sense;
static int count;
static int count_max;

static int debug_level = 0;

void gtmp_init(int num_threads){
    count_max = num_threads;
    sense = 0;
    count = count_max;
}

void gtmp_barrier(int thread_id){
    if(debug_level >= 1) 
        printf("[PROG %d] stared.\n", thread_id);

    bool old = sense;
    
    if(__sync_fetch_and_sub(&count, 1) == 1){
        if(debug_level >= 1) 
            printf("[PROG %d] last.\n", thread_id);
        count = count_max;
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

void gtmp_finalize(){

}

