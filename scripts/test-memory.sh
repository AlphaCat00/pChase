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
exp=2
#
# Benchmark
#

echo Benchmark initiated at $(date +%Y%m%d-%H%M) | tee -a chase.log

$pchase -o hdr | tee $output
if [ $exp == 0 ] ; then

    for mem_op in none "load 1" "store 1" load_all store_all
    do
        for access in random forward  # reverse
        do
            if [ $access != "random" ] ; then
                strides="1 2 4 8 16"
            else 
                strides="1"
            fi
            for stride in $strides ; do 
                for thread in `seq 1 $thread_num`; do
                    for i in `seq 1 $thread`; do
                        if [ $i == 1 ] ; then
                            t_map="0:$mem_bind"
                        else
                            t_map="${t_map};0:$mem_bind"
                        fi                
                    done         
                    access_=$access   
                    if [ $access != "random" ] ; then
                        access_="$access $stride"
                    fi 
                    mem_op_=$mem_op
                    if [ $mem_op == "load_all" ] ; then
                        mem_op_="load $stride"
                    elif [ $mem_op == "store_all" ] ; then
                        mem_op_="store $stride"
                    fi
                    $pchase -c 32m -p 32m -m $mem_op_ -a $access_ -s 1.0 -t $thread -n map $t_map -o csv | tee -a $output
                done
            done
        done
    done

elif [ $exp == 1 ] ; then
    for mem_op in none "load 1" "store 1" "iter_load 1" "iter_store 1"; do
        for stride in 1 2 4 8 16 ; do 
            for loop_size in 0 5 25 125 625 3125; do
                $pchase -c 32m -p 32m -g $loop_size -m $mem_op -a forward $stride -n map "0:$mem_bind" -s 3.0 -o csv | tee -a $output
            done
        done
    done
    python3 ../convert.py $output "memory operation,stride,loop length,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output
elif [ $exp == 2 ] ; then  
# random access with different sizes
    for mem_op in "load" "store"; do
        for op_size in 1 2 4 8 16 ; do 
            for thread in `seq 1 $thread_num`; do
                for i in `seq 1 $thread`; do
                    if [ $i == 1 ] ; then
                        t_map="0:$mem_bind"
                    else
                        t_map="${t_map};0:$mem_bind"
                    fi                
                done
                $pchase -c 32m -p 32m -m $mem_op $op_size -a random -s 3.0 -t $thread -n map $t_map -o csv | tee -a $output
            done
        done
    done    
    python3 ../convert.py $output "memory operation,operation size,access pattern,number of threads,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output
fi
echo Benchmark ended at $(date +%Y%m%d-%H%M) | tee -a chase.log

