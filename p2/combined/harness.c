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

        for (int i = 0; i < num_barriers; i++) {
            if (debug_level >= 1)
                printf("[HARNESS %d %d-%d] BARRIER START.\n", i, my_id, thread_num);

            if(which_method_clock == 1)
                gettimeofday(&begin_timeval, NULL);
            else
                clock_gettime(CLOCK_MONOTONIC, &begin_timespec);
            combined_barrier(thread_num);

            if(which_method_clock == 1)
                gettimeofday(&end_timeval, NULL);
            else
                clock_gettime(CLOCK_MONOTONIC, &end_timespec);

            if(which_method_clock == 1)
                report = (end_timeval.tv_sec*0.1 - begin_timeval.tv_sec*0.1)*(1e6) + 
                    (end_timeval.tv_usec*0.1 - begin_timeval.tv_usec*0.1);
            else
                report = (end_timespec.tv_sec*0.1 - begin_timespec.tv_sec*0.1)*(1e6) + 
                    (end_timespec.tv_nsec*0.1 - begin_timespec.tv_nsec*0.1)*(1e-3);

            if (debug_level >= 1)
                printf("[HARNESS %d %d %d] BARRIER END.\n", i, my_id, thread_num);
            printf("%d,%d,%d,%3.10f\n", i, my_id, thread_num, report);
        }
    }

    combined_finalize();

    MPI_Finalize();

    return 0;
}