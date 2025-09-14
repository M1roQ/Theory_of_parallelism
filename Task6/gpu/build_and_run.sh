#!/bin/bash

set -e

mkdir -p build profiles
cmake -B build
cmake --build build

ACCURACY=1e-6
ITER=1000000

SIZE=128
echo "Размер=$SIZE"
./build/gpu --size $SIZE --accuracy $ACCURACY --iterations $ITER

SIZE=256
echo "Размер=$SIZE"
./build/gpu --size $SIZE --accuracy $ACCURACY --iterations $ITER

SIZE=512
echo "Размер=$SIZE"
./build/gpu --size $SIZE --accuracy $ACCURACY --iterations $ITER

SIZE=1024
echo "Размер=$SIZE"
./build/gpu --size $SIZE --accuracy $ACCURACY --iterations $ITER
