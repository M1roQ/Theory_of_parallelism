#!/bin/bash

set -e

cmake -B build

cmake --build build

cd build

./MatrixVectorBenchmark

cd ..

python3 graphics.py -d results/matrix_benchmark.csv -s results/matrix_result.png