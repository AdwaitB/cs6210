#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "gtmpi.h"

static int debug_level = 1;
static int rank;
static int count_max;
static int rounds;
static int msg;
static MPI_Request send_status;
static MPI_Status recv_status;

void gtmpi_init(int num_processes, int id){
    rank = id;
    count_max = num_processes;
    rounds = ceil(log(count_max)/log(2));

    if(debug_level >= 1)
        printf("[INIT %d]\n", rank);
}

void gtmpi_barrier(){
    if(debug_level >= 1)
        printf("[PROG %d]\n", rank);

    for(int round = 0; round < rounds; round++){
        if(debug_level >= 1) 
            printf("[PROG %d] [ROUND %d] started.\n", rank, round);

        int communicate_to = (rank + (1<<round))%count_max;
        int receive_from = (rank - (1<<round) + count_max)%count_max;
        
        MPI_Isend(&msg, 1, MPI_INT, communicate_to, 0, MPI_COMM_WORLD, &send_status);
        
        if(debug_level >= 1) 
            printf("[PROG %d] [ROUND %d] waiting from %d.\n", rank, round, receive_from);
        
        MPI_Recv(&msg, 1, MPI_INT, receive_from, 0, MPI_COMM_WORLD, &recv_status);
        

        if(debug_level >= 1) 
            printf("[PROG %d] [ROUND %d] completed.\n", rank, round);
    }

    if(debug_level >= 1) 
        printf("[PROG %d] completed.\n", rank);
}

void gtmpi_finalize(){
    if(debug_level >= 1)
        printf("[FINALIZE %d]\n", rank);
}
