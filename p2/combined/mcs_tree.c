#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "mcs_tree.h"

#define ARRIVAL_DEGREE 4
#define WAKEUP_DEGREE 2

static int debug_level = 1;

int count_max;
int msg = 0;
static MPI_Request send_status;
static MPI_Status recv_status;

// arrival tree
static int rank;
static int arrival_parent;
static int arrival_child[ARRIVAL_DEGREE];

// wakeup tree
static int inform_parent;
static int inform_child[WAKEUP_DEGREE];

void mcstree_init(int num_processes, int id){
    if(debug_level >= 1)
        printf("[MCSTREE %d] init.\n", id);

    count_max = num_processes;

    rank = id;
    arrival_parent = rank == 0 ? 0 : (rank - 1)/ARRIVAL_DEGREE;

    for(int i = 0; i < ARRIVAL_DEGREE; i++){
        int val = id*ARRIVAL_DEGREE + 1 + i;
        arrival_child[i] = val < count_max ? val : -1;
    }

    inform_parent = rank == 0 ? 0 : (rank - 1)/WAKEUP_DEGREE;
    for(int i = 0; i < WAKEUP_DEGREE; i++){
        int val = id*WAKEUP_DEGREE + 1 + i;
        inform_child[i] = val < count_max ? val : -1;
    }
    
    if(debug_level >= 1){
        printf("[MCSTREE %d] arrival parent : %d, arrival_child : ", rank, arrival_parent);
        for(int i = 0; i < ARRIVAL_DEGREE; i++)
            printf("%d ", arrival_child[i]);
        
        printf(", inform_parent %d, inform_child : ", inform_parent);
        for(int i = 0; i < WAKEUP_DEGREE; i++)
            printf("%d ", inform_child[i]);
        
        printf("\n");
    }
}

void arrival(){
    if(debug_level >= 1)
        printf("[MCSTREE %d] arrival start.\n", rank);
    
    for(int i = 0; i < ARRIVAL_DEGREE; i++){
        if(arrival_child[i] == -1) 
            continue;
        MPI_Recv(&msg, 1, MPI_INT, arrival_child[i], 0, MPI_COMM_WORLD, &recv_status);
    }

    if(debug_level >= 1)
        printf("[MCSTREE %d] arrival propogate.\n", rank);

    if(rank != 0)
        MPI_Isend(&msg, 1, MPI_INT, arrival_parent, 0, MPI_COMM_WORLD, &send_status);

    if(debug_level >= 1) 
        printf("[MCSTREE %d] arrival completed.\n", rank);
}

void wakeup(){
    if(debug_level >= 1)
        printf("[MCSTREE %d] wakeup start.\n", rank);

    if(rank != 0)
        MPI_Recv(&msg, 1, MPI_INT, inform_parent, 0, MPI_COMM_WORLD, &recv_status);

    if(debug_level >= 1)
        printf("[MCSTREE %d] wakeup propogate.\n", rank);

    for(int i = 0; i < WAKEUP_DEGREE; i++){
        if(inform_child[i] == -1)  
            continue;
        MPI_Isend(&msg, 1, MPI_INT, inform_child[i], 0, MPI_COMM_WORLD, &send_status);
    }

    if(debug_level >= 1) 
        printf("[MCSTREE %d] wakeup completed.\n", rank);
}

void mcstree_barrier(){
    if(debug_level >= 1)
        printf("[MCSTREE %d] start.\n", rank);

    arrival();
    wakeup();

    if(debug_level >= 1) 
        printf("[MCSTREE %d] completed.\n", rank);
}

void mcstree_finalize(){
    if(debug_level >= 1)
        printf("[MCSTREE %d] finalized.\n", rank);
}
