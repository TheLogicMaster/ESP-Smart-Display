#!/bin/bash
 
# Combine FS and Firmware binaries to create flashable binary

firmwareSize=$(wc -c $1 | awk '{print $1}')
fallocate -l $(expr $((0x150000)) + $((0x150000)) - $firmwareSize) ./empty.bin # Values from partitions.csv
cat $1 ./empty.bin $2 > $3
rm ./empty.bin
