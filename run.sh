#!/bin/bash

data=("clean-soc-pokec-relationships.txt"  "GermanyRoadud.txt" "clean-soc-sinaweibo.txt" "USAud.txt" "clean-soc-twitter.txt")
if [ -z "$1" ]
then
    lines=2000
else
    lines=$1
fi

mpic++ main.cc

for graph in ${data[@]}
do
    echo "Starting ${graph//\.txt/}"
    head -n $lines ./data/$graph > ./data/${graph//\.txt/}$lines.txt
    mpirun --use-hwthread-cpus -c 8 a.out ./data/${graph//\.txt/}${lines}.txt 8
done
