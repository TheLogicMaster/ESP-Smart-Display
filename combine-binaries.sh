#!/bin/bash
 
# Combine FS and Firmware binaries to create flashable binary

firmwareSize=$(wc -c $1 | awk '{print $1}')
fallocate -l $(expr 1052688 + 1044464 - $firmwareSize) ./empty.bin # Values from eagle file
cat $1 ./empty.bin $2 > $3
rm ./empty.bin
