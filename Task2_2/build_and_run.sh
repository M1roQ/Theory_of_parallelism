#!/bin/bash

set -e

if [ ! -d "results" ]; then
    mkdir results
fi

cmake -B build

cmake --build build

cd build

./MyIntegration

cd ..

python3 graphics.py -d results/integration_speedup.csv -s results/integration_result.png