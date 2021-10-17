#include <stdbool.h>
#ifndef GTMP_H
#define GTMP_H

extern bool sense[32], parity[32];

void gtmp_init(int num_threads);
void gtmp_barrier();
void gtmp_finalize();
#endif
