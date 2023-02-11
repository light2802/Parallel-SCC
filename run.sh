#!/bin/bash

if [ -z "$1" ]
then
    lines=20000
else
    lines=$1
fi

head -n $lines ./data/clean-soc-pokec-relationships.txt > ./data/simple${lines}.txt
mpic++ main.cc
mpirun --use-hwthread-cpus -c 8 a.out ./data/simple${lines}.txt 8
