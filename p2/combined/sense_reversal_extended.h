#include <stdbool.h>
#ifndef SENSE_REVERSAL_H
#define SENSE_REVERSAL_H

void sense_reversal_extended_init(int num_threads);
void sense_reversal_extended_barrier(int thread_id);
void sense_reversal_extended_finalize();

#endif
