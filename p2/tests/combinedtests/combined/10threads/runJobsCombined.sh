#!/bin/bash
for i in 2
do
    qsub p$i.pbs
done
