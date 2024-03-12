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

pchase=../../build/release/chase 

mem_bind=1
mem_bind_1=2
thread_num=4
exp=10
#
# Benchmark
#

echo Benchmark initiated at $(date +%Y%m%d-%H%M) | tee -a chase.log

$pchase -o hdr | tee $output
if [ $exp == 0 ] ; then

    for mem_op in none "load 1" "store 1"  # load_all store_all
    do
        for access in random # forward  # reverse
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
                    if [ "$mem_op" == "load_all" ] ; then
                        mem_op_="load $stride"
                    elif [ "$mem_op" == "store_all" ] ; then
                        mem_op_="store $stride"
                    fi
                    $pchase -c 32m -p 32m -m $mem_op_ -a $access_ -s 3.0 -t $thread -n map $t_map -o csv | tee -a $output
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
        for op_size in 1 2 4 8 16 32 64; do 
            for thread in `seq 1 $thread_num`; do
                for i in `seq 1 $thread`; do
                    if [ $i == 1 ] ; then
                        t_map="0:$mem_bind"
                    else
                        t_map="${t_map};0:$mem_bind"
                    fi                
                done
                $pchase -c 128m -p 128m -m $mem_op $op_size -a random -s 3.0 -t $thread -n map $t_map -o csv | tee -a $output
            done
        done
    done    
    python3 ../convert.py $output "memory operation,operation size,access pattern,number of threads,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output
elif [ $exp == 3 ] ; then  
    for mem_op in "load" "store"; do
        for loop_size in 0 5 25 125 625 3125; do
            for op_size in 1 2 4 8 16 ; do 
                $pchase -c 32m -p 32m -g $loop_size -m $mem_op $op_size -a random -s 3.0 -n map "0:$mem_bind" -o csv | tee -a $output
            done
        done
    done    
    python3 ../convert.py $output "memory operation,operation size,access pattern,loop length,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output

elif [ $exp == 4 ] ; then
    for mem_op in "load" "store"; do
        for op_size in 1 2 4 8 16 32 64; do 
            for percentage in 0.99 0.95 0.9 0.8 0.7 0.6 0.5 0.4 0.3 0.2 0.1 0.05 0.01; do
                $pchase -c 32m -p 32m -m $mem_op $op_size -a interleaved $percentage -n map "0:$mem_bind" -s 3.0 -o csv | tee -a $output
            done
        done
    done
    python3 ../convert.py $output "memory operation,operation size,access pattern,interleaved percentage,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output


elif [ $exp == 5 ] ; then
    for mem_op in "load" "store"; do
        for chain_size in 4k 8k 16k 32k 64k 128k 256k 512k 1m 2m 4m 8m 16m 32m; do 
                $pchase -c $chain_size -p $chain_size -m $mem_op 1 -a random -n map "0:$mem_bind" -s 3.0 -o csv | tee -a $output

        done
    done
    python3 ../convert.py $output "chain size (bytes),memory operation,access pattern,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output

elif [ $exp == 6 ] ; then
    python3 ../multi_exp.py $pchase $mem_bind $output
elif [ $exp == 7 ] ; then
    python3 ../multi_exp_2.py $pchase $mem_bind $output
elif [ $exp == 8 ] ; then
    python3 ../multi_exp_3.py $pchase $mem_bind $output
elif [ $exp == 9 ] ; then
    for access in random forward  # reverse
    do
        for op_size in 1 2 4 8 16 ; do 
            for thread in `seq 1 $thread_num`; do
                for i in `seq 1 $thread`; do
                    if [ $i == 1 ] ; then
                        t_map="0:$mem_bind,$mem_bind_1"
                    else
                        t_map="${t_map};0:$mem_bind,$mem_bind_1"
                    fi                
                done  
                access_=$access   
                if [ $access != "random" ] ; then
                    access_="$access $op_size"
                fi 
                $pchase -c 32m -p 32m -a $access_ -m copy $op_size -s 3.0 -n map $t_map -o csv | tee -a $output
            done
        done
    done
    python3 ../convert.py $output "memory operation,operation size,access pattern,number of threads,memory latency (ns),memory bandwidth (MB/s)" > simplified_$output
elif [ $exp == 10 ] ; then
    python3 ../cache_pollution.py $pchase $mem_bind $mem_bind_1 $output
fi

echo Benchmark ended at $(date +%Y%m%d-%H%M) | tee -a chase.log

