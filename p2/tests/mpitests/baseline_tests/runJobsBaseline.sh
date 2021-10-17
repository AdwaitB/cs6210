#!/bin/bash
for i in 2 4 6 8 10 12
do
    qsub baseline_$i.pbs
done
