MPICC = mpicc
CFLAGS = -g -Wall -std=gnu99 -I.
LDLIBS = -lm

MP_SRC0 = baseline.c
MP_SRC1 = gtmpi1.c
MP_SRC2 = gtmpi2.c

all: mpi1 mpi2

baseline: baseline.c harness.o
	$(MPICC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

mpi1: gtmpi1.c harness.o
	$(MPICC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

mpi2: gtmpi2.c harness.o
	$(MPICC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS)

%.o: %.c
	$(MPICC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *.dSYM mpi1 mpi2 baseline

