#include <stdio.h>
#include <omp.h>
#include <time.h>
#include <sys/time.h>
#include "mpi.h"
#include "combined.h"

#define NUM_THREADS 3

static int debug_level = 0;
static int num_barriers = 100;

int which_method_clock = 1;

struct timeval begin_timeval, end_timeval;
struct timespec begin_timespec, end_timespec;
clock_t begin_clock, end_clock;
double report;

int main(int argc, char ** argv) {
    int my_id, num_processes, thread_num; //, num_threads;
    MPI_Init( & argc, & argv);

    MPI_Comm_size(MPI_COMM_WORLD, & num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, & my_id);

    omp_set_num_threads(NUM_THREADS);

    combined_init(NUM_THREADS, num_processes, my_id);

    #pragma omp parallel private(thread_num)
    {
        thread_num = omp_get_thread_num();

        if(which_method_clock == 1)
            gettimeofday(&begin_timeval, NULL);
        else if(which_method_clock == 2)
            clock_gettime(CLOCK_MONOTONIC, &begin_timespec);
        else if(which_method_clock == 3)
            begin_clock = clock();

        for (int i = 0; i < num_barriers; i++) {
            if (debug_level >= 1)
                printf("[HARNESS %d %d-%d] BARRIER START.\n", i, my_id, thread_num);

            combined_barrier(thread_num);

            if (debug_level >= 1)
                printf("[HARNESS %d %d %d] BARRIER END.\n", i, my_id, thread_num);
            
        }

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

        printf("%d,%d,%d,%3.10f\n", -1, -1, thread_num, report);
    }

    combined_finalize();

    MPI_Finalize();

    return 0;
}