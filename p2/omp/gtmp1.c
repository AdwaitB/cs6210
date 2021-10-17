#include <stdio.h>
#include <omp.h>
#include <stdbool.h>
#include "gtmp.h"

static bool sense_local;
static int count;
static int count_max;

static int debug_level = 0;

void gtmp_init(int num_threads){
    count_max = num_threads;
    sense_local = false;
    count = count_max;
}

void gtmp_barrier(){
    int thread_id = omp_get_thread_num();

    if(debug_level >= 1) 
        printf("[PROG %d] stared.\n", thread_id);

    bool old = sense_local;
    
    if(__sync_fetch_and_sub(&count, 1) == 1){
        if(debug_level >= 1) 
            printf("[PROG %d] last.\n", thread_id);
        count = count_max;
        sense_local = !old;
    }
    else{
        if(debug_level >= 1) 
            printf("[PROG %d] not last.\n", thread_id);
        while(sense_local == old);
    }
    
    if(debug_level >= 1) 
        printf("[PROG %d] completed.\n", thread_id);
}

void gtmp_finalize(){

}

