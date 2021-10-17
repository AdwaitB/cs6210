#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

#define ARRIVAL_DEGREE 4
#define WAKEUP_DEGREE 2

static int debug_level = 0;

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

void gtmpi_init(int num_processes){
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(debug_level >= 1)
        printf("[PROG %d] init.\n", rank);

    count_max = num_processes;

    arrival_parent = rank == 0 ? 0 : (rank - 1)/ARRIVAL_DEGREE;

    for(int i = 0; i < ARRIVAL_DEGREE; i++){
        int val = rank*ARRIVAL_DEGREE + 1 + i;
        arrival_child[i] = val < count_max ? val : -1;
    }

    inform_parent = rank == 0 ? 0 : (rank - 1)/WAKEUP_DEGREE;
    for(int i = 0; i < WAKEUP_DEGREE; i++){
        int val = rank*WAKEUP_DEGREE + 1 + i;
        inform_child[i] = val < count_max ? val : -1;
    }
    
    if(debug_level >= 1){
        printf("[PROG %d] arrival parent : %d, arrival_child : ", rank, arrival_parent);
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
        printf("[PROG %d] arrival start.\n", rank);
    
    for(int i = 0; i < ARRIVAL_DEGREE; i++){
        if(arrival_child[i] == -1) 
            continue;
        MPI_Recv(&msg, 1, MPI_INT, arrival_child[i], 0, MPI_COMM_WORLD, &recv_status);
    }

    if(debug_level >= 1)
        printf("[PROG %d] arrival propogate.\n", rank);

    if(rank != 0)
        MPI_Isend(&msg, 1, MPI_INT, arrival_parent, 0, MPI_COMM_WORLD, &send_status);

    if(debug_level >= 1) 
        printf("[PROG %d] arrival completed.\n", rank);
}

void wakeup(){
    if(debug_level >= 1)
        printf("[PROG %d] wakeup start.\n", rank);

    if(rank != 0)
        MPI_Recv(&msg, 1, MPI_INT, inform_parent, 0, MPI_COMM_WORLD, &recv_status);

    if(debug_level >= 1)
        printf("[PROG %d] wakeup propogate.\n", rank);

    for(int i = 0; i < WAKEUP_DEGREE; i++){
        if(inform_child[i] == -1)  
            continue;
        MPI_Isend(&msg, 1, MPI_INT, inform_child[i], 0, MPI_COMM_WORLD, &send_status);
    }

    if(debug_level >= 1) 
        printf("[PROG %d] wakeup completed.\n", rank);
}

void gtmpi_barrier(){
    if(debug_level >= 1)
        printf("[PROG %d] start.\n", rank);

    arrival();
    wakeup();

    if(debug_level >= 1) 
        printf("[PROG %d] completed.\n", rank);
}

void gtmpi_finalize(){
    if(debug_level >= 1)
        printf("[FINALIZE %d]\n", rank);
}
