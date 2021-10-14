#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "gtmpi.h"

static int debug_level = 0;

int main(int argc, char** argv)
{
  int num_processes, num_rounds = 1;

  MPI_Init(&argc, &argv);
  
  if (argc < 2){
    fprintf(stderr, "Usage: ./harness [NUM_PROCS]\n");
    exit(EXIT_FAILURE);
  }

  num_processes = strtol(argv[1], NULL, 10);
  
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  gtmpi_init(num_processes, rank);
  
  int k;
  for(k = 0; k < num_rounds; k++){
    if(debug_level >= 1) 
      printf("[HARNESS %d] Starting %d barrier.\n", rank, k);
    gtmpi_barrier();
    if(debug_level >= 1) 
      printf("[HARNESS %d] Done %d barrier.\n", rank, k);
  }

  gtmpi_finalize();  

  MPI_Finalize();

  return 0;
}
