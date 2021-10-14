#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>
#include <time.h>
#include <sys/time.h>
#include "gtmp.h"

static int debug_level = 0;

struct timeval begin_timeval, end_timeval;
struct timespec begin_timespec, end_timespec;
double report;

int main(int argc, char ** argv) {
    // Harness initializes here
    int num_threads, num_barriers = 100;

    if (argc < 2) {
        fprintf(stderr, "Usage: ./harness [NUM_THREADS]\n");
        exit(EXIT_FAILURE);
    }
    num_threads = strtol(argv[1], NULL, 10);

    omp_set_dynamic(0);
    if (omp_get_dynamic())
        printf("Warning: dynamic adjustment of threads has been set\n");

    omp_set_num_threads(num_threads);

    gtmp_init(num_threads);
    if (debug_level >= 1)
        printf("[HARNESS main] Starting harness with %d threads and %d barriers.\n", num_threads, num_barriers);

    #pragma omp parallel shared(num_threads) 
    {
        int thread_id = omp_get_thread_num();
        if (debug_level >= 1)
            printf("[HARNESS %d] Starting thread out of %d.\n", thread_id, num_threads);
        int i;
        for (i = 0; i < num_barriers; i++) {
            if (debug_level >= 1)
                printf("[HARNESS %d] Barrier %d started.\n", thread_id, i);
            
            //gettimeofday(&begin_timeval, NULL);
            clock_gettime(CLOCK_MONOTONIC, &begin_timespec);

            gtmp_barrier(thread_id);

            //gettimeofday(&end_timeval, NULL);
            clock_gettime(CLOCK_MONOTONIC, &end_timespec);

            //report = (end_timeval.tv_sec*0.1 - begin_timeval.tv_sec*0.1)*(1e6) + 
                // (end_timeval.tv_usec*0.1 - begin_timeval.tv_usec*0.1); // gettimeofday
            report = (end_timespec.tv_sec*0.1 - begin_timespec.tv_sec*0.1)*(1e6) + 
                (end_timespec.tv_nsec*0.1 - begin_timespec.tv_nsec*0.1)*(1e-3); // clock_gettime
            
            if (debug_level >= 1)
                printf("[HARNESS %d] Barrier %d ended.\n", thread_id, i);
            printf("%d,%d,%3.10f\n", i, thread_id, report);
        }
    }

    gtmp_finalize();

    return 0;
}
