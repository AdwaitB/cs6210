#include <stdio.h>
#include <omp.h>
#include <math.h>
#include <stdlib.h>
#include "gtmp.h"

static int rounds;
static int count_max;
static bool *** visited;

static int debug_level = 0;

bool sense[32], parity[32];

void clear_visited(){
    for(int i = 0; i < rounds; i++){
        for(int j = 0; j < count_max; j++)
            visited[i][j][0] = visited[i][j][1] = false;
    }
}

void gtmp_init(int num_threads){
    count_max = num_threads;
    rounds = ceil(log(count_max)/log(2));

    visited = (bool***) malloc(sizeof(bool**)*rounds);
    for(int i = 0; i < rounds; i++){
        visited[i] = (bool**) malloc(sizeof(bool*)*count_max);
        for(int j = 0; j < count_max; j++)
            visited[i][j] = (bool*) malloc(sizeof(bool)*2);
    }
    
    clear_visited();
}

void gtmp_barrier(){
    int thread_id = omp_get_thread_num();

    if(debug_level >= 1) 
        printf("[PROG %d] stared.\n", thread_id);

    for(int round = 0; round < rounds; round++){
        if(debug_level >= 1) 
            printf("[PROG %d] [ROUND %d] started.\n", thread_id, round);

        int communicate_to = (thread_id + (1<<round))%count_max;
        visited[round][communicate_to][parity[thread_id] ? 1 : 0] = !sense[thread_id];
        
        if(debug_level >= 1) 
            printf("[PROG %d] [ROUND %d] spinning.\n", thread_id, round);

        while(visited[round][thread_id][parity[thread_id] ? 1 : 0] == sense[thread_id]);

        if(debug_level >= 1) 
            printf("[PROG %d] [ROUND %d] completed.\n", thread_id, round);
    }

    if(parity[thread_id]) sense[thread_id] = !sense[thread_id];
    parity[thread_id] = !(parity[thread_id]);

    if(debug_level >= 1) 
        printf("[PROG %d] completed.\n", thread_id);
}

void gtmp_finalize(){
    for(int i = 0; i < rounds; i++){
        for(int j = 0; j < count_max; j++)
            free(visited[i][j]);
        free(visited[i]);
    }
    free(visited);
}

