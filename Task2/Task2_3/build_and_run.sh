#!/bin/bash

set -e

if [ ! -d "results" ]; then
    mkdir results
fi

cmake -B build

cmake --build build

cd build

./iteration_method

cd ..

python3 graphics.py -d results/sole_result.csv -s results/iteration_result.png