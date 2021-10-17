#!/bin/bash
for i in 2 4 6
do
    qsub p$i.pbs
done
