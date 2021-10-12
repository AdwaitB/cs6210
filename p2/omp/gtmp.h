#include <stdbool.h>
#ifndef GTMP_H
#define GTMP_H

void gtmp_init(int num_threads);
void gtmp_barrier(int thread_id);
void gtmp_finalize();
#endif
