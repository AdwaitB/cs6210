
## Project Instruction Updates:

1. Complete the function MemoryScheduler() in memory_coordinator.c
2. If you are adding extra files, make sure to modify Makefile accordingly.
3. Compile the code using the command `make all`
4. You can run the code by `./memory_coordinator <interval>`

## Notes:

1. Make sure that the VMs and the host has sufficient memory after any release of memory.
2. Memory should be released gradually. For example, if VM has 300 MB of memory, do not release 200 MB of memory at one-go.
3. Domain should not release memory if it has less than or equal to 100MB of unused memory.
4. Host should not release memory if it has less than or equal to 200MB of unused memory.
5. While submitting, write your algorithm and logic in this Readme.

## Algorithm

### Notations and Heuristics

> Realistic value of how much a process might need in 1 sec.

Let us take an example. Say the running program has a custom data structure that takes $S_{ds}$ bytes of memory. In practical cases, considering on an average an $O(log(n))$ complexity, the program in less than 1 second can only process $10^6$ such data structures. So the amount of memory it needs in a second should be less than $S_{ds}*10^6 B$ = $S_{ds} MB$ of memory.

For practical consideration let us say a practical class can store a max of $100B$, then the total required memory free in an OS should be $100 MB$.

**Heuristic 0**: Realistic memory usage of a process in a second.

**H00**: 100MB.

> The buffer value $S_b$ is the minumum amount of memory that should remain unused in a machine.

One more thing to consider from the realistic value is the number of demanding processes in an OS. If we consider that there are 2 demanding processes in an OS, then we will need $200 MB$ of memory. It will be logical if we scale the $S_b$ based on the current memory requirements.

**Heuristic 1**: Value of $S_b$.

**H11**: $S_b$ is 100MB.

**H12**: $S_b$ is the maximum of 100MB and 10% of the current memory usage.

> The acceptable variance $V$ is the amount in percent around that buffer that marks the grey area of memory variation.

The unused value in an operating system will keep on varying given the large number of services running, so we need to have a variance threshold to stop our algorithm from flipping between giving and taking back memory based on small changes in the unused value.

**Heuristic 2**: Variance threshold

**H21**: $V$ = 10%.

> How much memory should be give once we know a machine gives more memory.

Now we know that a machine needs more memory, how much memory should be given. An OS designer can never estimate this value. A process might need only a small amount or might request in GBs if its loading say a dataset. Giving the whole available memory is also not practical as claiming committed memory back might be nonintuitive from the programmer's size. Similar to the buffer size, we have to have an average expectation analysis. The most trivial case is we give a fixed size every second or give estimated into the time interval per iteration. So if we expect a process to use 100MB per second and the interval for the memory manager is 2 sec then we give out 200MB of data.

**Heuristic 3**: How much memory to give.

**H31**: 32 MB per iteration.
distance
**H32**: $S_b$ per iteration. 

**H33**: $S_b*interval$ per iteration where interval is the interval between memory runs.

> How do we know that a machine has extra memory that is unused.

A machine when is done with a set of resources might release a lot of memory at once. This can happen if a process is over or gets crashes. This can also happen if a process is done using some part of processing and is getting ready to do another task which again might need the same amount back. An idea is to wait for the process to take back the memory. If it does not, then we can start taking back.

**Heuristic 4**: Should the host take back the memory.

**H41**: Yes if its greater than $S_b*(1+V)$

**H42**: Yes if its greater than $S_b*(1_V)$ for the last T iterations.

> How do we know how much to take back 

Now we know that we have to take back memory from an application, how much should we take that. We should do this to balance the amount of free memory and make it available to other vms. 

**Heuristic 5**: How much memory should be taken back.

**H51**: 32 MB per iteration.

**H52**: 25% of distance from buffer.

**H53**: 25% of distance from buffer if this the distance is more than $4*S_b$ else $S_b*interval*2*V$

> Host does not have any memory to give out, from where does it take to satisfy.

This will be refered to as the balancing act. Idea here is that we have a state where after reclaiming memory and giving it back, the host does not have any memory and still there is a vm that requires memory. There are some machines which are needy, some which are not needy. If we take 

**Heuristic 6**: Which machine to take memory from if the host is full and some machine is needy.

**H60**: Do not do anything. We do not gurantee fairness anywhere.

**H61**: We take some memory from the non-needy machine with the highest used memory and give it to the needy ones. If no non-needy machines exist, we take out memory from the machine with the highest used memory. Fair share is assumed here.

### Algorithm Sketch

Restrictions:
1. A machine cannot have less than 200 MB memory.
2. A machine including the host must have atleast 100MB free memeory.

This algorithm considers the following heuristics: H00, H12, H21, H33, H42, H53, H60.

Here are the exact steps for the algorithm.
1. The algorithm first starts by calculating the expected buffer space for every node. H00, H12, H21.
2. Then it calculates the amount of memory that is needed so that atleast this buffer space is achieved.
3. By hueristics, we say that a machine will have negative distance if it is going below memory and positive distance if it has more memory.
4. Based on this value, memory either is given back or taken from the machine. H33, H42, H53.
5. Since there is no fairness in share or any balancer, we just let the domains starve if the host or any machines have no memory to space. H60.
