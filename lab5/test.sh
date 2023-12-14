#!/bin/bash

declare -a arr=(15)

for number in `seq 1 50`
do
    for num in "${arr[@]}"
    do
    echo "$num"
    ./build/pa5 --mutexl -p "$num"

    done
done
# LD_PRELOAD=/home/marsen/itmo/distributed_computing/lab5/pa5/lib64/libruntime.so 