#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <sys/time.h>
#include "gtmpi.h"

static int debug_level = 0;

int which_method_clock = 1;

struct timeval begin_timeval, end_timeval;
struct timespec begin_timespec, end_timespec;
clock_t begin_clock, end_clock;
double report;

int main(int argc, char ** argv) {
    int num_processes, num_rounds = 100;

    MPI_Init( & argc, & argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: ./harness [NUM_PROCS]\n");
        exit(EXIT_FAILURE);
    }

    num_processes = strtol(argv[1], NULL, 10);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, & rank);

    gtmpi_init(num_processes, rank);

    int k;
    for (k = 0; k < num_rounds; k++) {
        if (debug_level >= 1)
            printf("[HARNESS %d] Starting %d barrier.\n", rank, k);
        
        if(which_method_clock == 1)
            gettimeofday(&begin_timeval, NULL);
        else if(which_method_clock == 2)
            clock_gettime(CLOCK_MONOTONIC, &begin_timespec);
        else if(which_method_clock == 3)
            begin_clock = clock();

        gtmpi_barrier();

        if(which_method_clock == 1)
            gettimeofday(&end_timeval, NULL);
        else if(which_method_clock == 2)
            clock_gettime(CLOCK_MONOTONIC, &end_timespec);
        else if(which_method_clock == 3)
            end_clock = clock();

        if(which_method_clock == 1)
            report = (end_timeval.tv_sec*0.1 - begin_timeval.tv_sec*0.1)*(1e6) + 
                (end_timeval.tv_usec*0.1 - begin_timeval.tv_usec*0.1);
        else if(which_method_clock == 2)
            report = (end_timespec.tv_sec*0.1 - begin_timespec.tv_sec*0.1)*(1e6) + 
                (end_timespec.tv_nsec*0.1 - begin_timespec.tv_nsec*0.1)*(1e-3);
        else if(which_method_clock == 3)
            report = ((double) (end_clock - begin_clock))*(1e6)/CLOCKS_PER_SEC;

        if (debug_level >= 1)
            printf("[HARNESS %d] Done %d barrier.\n", rank, k);
        printf("%d,%d,%3.10f\n", k, rank, report);
    }

    gtmpi_finalize();

    MPI_Finalize();

    return 0;
}
