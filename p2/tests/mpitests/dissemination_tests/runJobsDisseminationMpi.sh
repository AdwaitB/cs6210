#!/bin/bash
for i in 2 4 6 8 10 12
do
    qsub dissemination_$i.pbs
done
