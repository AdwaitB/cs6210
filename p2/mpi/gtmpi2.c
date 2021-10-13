#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

#define ARRIVAL_DEGREE 4
#define WAKEUP_DEGREE 2

static int debug_level = 1;

int count_max;

// arrival tree
static int current;
static int parent;
static int arrival_child[ARRIVAL_DEGREE];

// wakeup tree
static int inform_child[WAKEUP_DEGREE];

void gtmpi_init(int num_processes, int id){
    if(debug_level >= 1)
        printf("[PROG %d] init.\n", current);

    count_max = num_processes;

    current = id;
    parent = current == 0 ? 0 : (current - 1)/ARRIVAL_DEGREE;

    for(int i = 0; i < ARRIVAL_DEGREE; i++){
        int val = id*ARRIVAL_DEGREE + 1 + i;
        arrival_child[i] = val < count_max ? val : -1;
    }

    for(int i = 0; i < WAKEUP_DEGREE; i++){
        int val = id*WAKEUP_DEGREE + 1 + i;
        inform_child[i] = val < count_max ? val : -1;
    }
    
    if(debug_level >= 1){
        printf("[PROG %d] parent : %d, arrival_child : ", current, parent);
        for(int i = 0; i < ARRIVAL_DEGREE; i++)
            printf("%d ", arrival_child[i]);
        
        printf(", inform_child : ");
        for(int i = 0; i < WAKEUP_DEGREE; i++)
            printf("%d ", inform_child[i]);
        
        printf("\n");
    }
}

void gtmpi_barrier(){
    
}

void gtmpi_finalize(){

}
