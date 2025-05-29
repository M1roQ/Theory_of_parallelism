#!/bin/bash

set -e

mkdir -p build profiles
cmake -B build
cmake --build build

ACCURACY=1e-6
ITER=4000

SIZE=512
echo "Размер=$SIZE"
nsys profile --stats=true --output=profiles/gpu_profile_4000_step2 ./build/gpu --size $SIZE --accuracy $ACCURACY --iterations $ITER
