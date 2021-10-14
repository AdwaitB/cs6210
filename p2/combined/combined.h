#include <stdio.h>

void combined_init(int num_threads, int num_processes, int rank);

void combined_barrier(int thread_id);

void combined_finalize();
