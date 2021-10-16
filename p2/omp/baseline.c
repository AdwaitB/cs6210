#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

void gtmp_init(int num_threads){}

void gtmp_barrier(int thread_id, bool* sense, bool* parity){
    #pragma omp barrier
}

void gtmp_finalize(){}

