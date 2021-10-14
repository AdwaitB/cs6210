#ifndef MCSTREE_H
#define MCSTREE_H

void mcstree_init(int num_processes, int id);
void mcstree_barrier();
void mcstree_finalize();

#endif
