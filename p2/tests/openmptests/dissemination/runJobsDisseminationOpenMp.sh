#!/bin/bash
for i in 2 3 4 5 6 7 8
do
    qsub dissemination_openmp$i.pbs
done
