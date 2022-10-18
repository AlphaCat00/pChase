#!/bin/sh

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
#
# Benchmark
#

echo Benchmark initiated at $(date +%Y%m%d-%H%M) | tee -a chase.log

$pchase -o hdr | tee $output
for access in random "forward 1" "forward 2" "forward 4" "forward 8" "forward 16" "reverse 1" "reverse 2" "reverse 4" "reverse 8" "reverse 16"
do
    for mem_op in none load store
    do
        $pchase -n add 1 -c 32m -m $mem_op -a $access -s 1.0 -o csv | tee -a $output
    done
done

echo Benchmark ended at $(date +%Y%m%d-%H%M) | tee -a chase.log

