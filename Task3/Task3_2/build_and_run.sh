#!/bin/bash

set -e

cmake -B build
cmake --build build

./build/TaskServerClient

./build/CheckResults results/pow.txt
./build/CheckResults results/sin.txt
./build/CheckResults results/sqrt.txt
