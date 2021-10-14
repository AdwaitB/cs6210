#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>
#include "gtmp.h"

static int debug_level = 0;

int main(int argc, char** argv)
{
  // Harness initializes here
  int num_threads, num_barriers = 20;

  if (argc < 2){
    fprintf(stderr, "Usage: ./harness [NUM_THREADS]\n");
    exit(EXIT_FAILURE);
  }
  num_threads = strtol(argv[1], NULL, 10);

  omp_set_dynamic(0);
  if (omp_get_dynamic())
    printf("Warning: dynamic adjustment of threads has been set\n");

  omp_set_num_threads(num_threads);

  gtmp_init(num_threads);
  if(debug_level >= 1) 
    printf("[HARNESS main] Starting harness with %d threads and %d barriers.\n", num_threads, num_barriers);


#pragma omp parallel shared(num_threads)
   {
     int thread_id = omp_get_thread_num();
     if(debug_level >= 1) 
      printf("[HARNESS %d] Starting thread out of %d.\n", thread_id, num_threads);
     int i;
     for(i = 0; i < num_barriers; i++){
       if(debug_level >= 1) 
        printf("[HARNESS %d] Barrier %d started.\n", thread_id, i);
       gtmp_barrier(thread_id);
       if(debug_level >= 1) 
        printf("[HARNESS %d] Barrier %d ended.\n", thread_id, i);
     }
   }

   gtmp_finalize();

   return 0;
}
