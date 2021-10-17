#!/bin/bash
for i in 2 4
do
    qsub p$i.pbs
done
