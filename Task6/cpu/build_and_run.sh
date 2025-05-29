#!/bin/bash

set -e

ACCURACY=1e-6
ITER=1000000

cmake -B build
cmake --build build

SIZE=128
echo "Размер=$SIZE"
./build/serial --size $SIZE --accuracy $ACCURACY --iterations $ITER
./build/multi  --size $SIZE --accuracy $ACCURACY --iterations $ITER

SIZE=256
echo "Размер=$SIZE"
./build/serial --size $SIZE --accuracy $ACCURACY --iterations $ITER
./build/multi  --size $SIZE --accuracy $ACCURACY --iterations $ITER

SIZE=512
echo "Размер=$SIZE"
./build/serial --size $SIZE --accuracy $ACCURACY --iterations $ITER
./build/multi  --size $SIZE --accuracy $ACCURACY --iterations $ITER

SIZE=1024
echo "Размер=$SIZE"
./build/multi  --size $SIZE --accuracy $ACCURACY --iterations $ITER
./build/serial --size $SIZE --accuracy $ACCURACY --iterations $ITER
