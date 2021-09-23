## Project Instruction Updates:

1. Complete the function CPUScheduler() in vcpu_scheduler.c
2. If you are adding extra files, make sure to modify Makefile accordingly.
3. Compile the code using the command `make all`
4. You can run the code by `./vcpu_scheduler <interval>`
5. While submitting, write your algorithm and logic in this Readme.

## Algorithm

### Notation and Heuristics

> What time to collect data

The CPU stats are absolute, hence to understand the pattern of a domain that is utilizing the vcpu, we will have to fix the methodology of sampling this time. A simple way is to check in the fixed time interval the difference in this absolute value for a domain and then assume that value for that interval. There will always be a tradeoff between taking an interval too large, which would dilute the immediate requirements to taking the interval too small, which would not correctly show how the domain is poerforming.

**Heuristic 1**: Method of collecting data.

**H11**: Take the interval size same as the scheduler runs. Since the scheduler has to run in a fixed time period which has to be small, we can take time between the consecutive runs of the scheduler.

> Extending data to the next interval in time.

Based on the past interval's data, the decision to make is how much processor will the domain use. If in the last interval (s) there is a increase, it is expected for the program to use more data. This type of dynamic nature will be expensive given the amount of data structures used, but will be most effective. A very simple extrapolation will be lighweight but not so accurate.

**Heuristic 2**: Predicting usage values for next interval.

**H21**: Simply use values from last interval.

**H22**: Use average values from last X intervals.

> Mapping vcpu to pcpu

Now that the expected values are there, the pin mappings need to be done. This is a standard *k-way number partitioning* where k is the number of pcpu. We have a list of expected usages of the domains in the next interval, we want to have a best fit so that all pcpus are not overloaded and (if possible) running on low and equally distributed loads.

**Heuristic 3**: Assigning vcpu to pcpu.

**H31**: Assign next vcpu (any order) to pcpu with the lowest expected usage.

**H32**: Assing next most used vcpu to pcpu with lowest expected usage.

> Balancing act

Let us say there are 16 domains on a 4 pcpu processor. All the domains are doing something very small and are usually idle. We do not want the vcpus to be shuffled here and there at every interval. So some balancing act should prevent excess changes in the pinnings. This is the most hard to achieve as it adds an extra penalty to moving vcpus from one pcpu to other.

**Heuristic 4**: How to minimize vcpu to pcpu pinning values with balancing.

**H41**: Do not change pinning if the distribution is less than 5% different. Wait for 3 intervals to change this pinning to prevent compounding of the expected distribution. This is very lighweight to implement as we just need to sort small arrays (size of pcpu) and then do a direct compare for the 5% boundary.


### Algorithm Sketch

The algorithm does not have a full fledged balancing act as it is too complex to implement in such a short notice.

1. First we get all the statistics for every vcpu utilization. For the host, we check how much each vcpu has used given the underlying pinning. This removes all the kernel space for the host while keeping the vcpu utilization accurate. H1, H2 used here
2. Based on the last interval usage, we then calculate the new pinnings. H3 used here.
3. We get expected new pcpu usages based on the pinnings and then check if there is more than 5% deviation from the last pinning. This is done by sorting the final values as all the pcpus are assumed to be the same.
4. If yes, then we use the new pinning.  
5. If no then we let the pinning stay like that for a maximum of 3 consecutive intervals as till then it is possible that the deviaions compound to more than 10%. H4.

