#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

void gtmpi_init(int num_processes){}

void gtmpi_barrier(){
    MPI_Barrier(MPI_COMM_WORLD);
}

void gtmpi_finalize(){}

