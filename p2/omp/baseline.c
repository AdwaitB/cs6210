#include <stdio.h>
#include <omp.h>
#include "gtmp.h"

void gtmp_init(int num_threads){}

void gtmp_barrier(){
    #pragma omp barrier
}

void gtmp_finalize(){}

