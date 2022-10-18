#!/bin/bash

#
# Initialisation
#

# Configurable variables
output=chase.csv

# Generate a timestamp
timestamp=$(date +%Y%m%d-%H%M)

# Create a temporary directory
mkdir chase-$timestamp
cd chase-$timestamp

# Save some system information
uname -a > kernel.txt
cat /proc/cpuinfo > cpuinfo.txt
cat /proc/meminfo > meminfo.txt

pchase=../../builddir/chase 

mem_bind=1
thread_num=4
#
# Benchmark
#

echo Benchmark initiated at $(date +%Y%m%d-%H%M) | tee -a chase.log

$pchase -o hdr | tee $output
for mem_op in none load store load_all store_all
do
    for access in random "forward 1" "forward 2" "forward 4" "forward 8" "forward 16" "reverse 1" "reverse 2" "reverse 4" "reverse 8" "reverse 16"
    do
        for thread in `seq 1 $thread_num`; do
            for i in `seq 1 $thread`; do
                if [ $i == 1 ] ; then
                    t_map="0:$mem_bind"
                else
                    t_map="${t_map};0:$mem_bind"
                fi                
            done
            $pchase -c 32m -p 32m -m $mem_op -a $access -s 3.0 -t $thread -n map $t_map -o csv | tee -a $output
        done
    done
done

echo Benchmark ended at $(date +%Y%m%d-%H%M) | tee -a chase.log

