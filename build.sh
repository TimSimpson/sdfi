#!/bin/bash

rm -rf build/gcc
mkdir -p build/gcc
cmake -H. -Bbuild/gcc -G "Unix Makefiles"
cmake -H. -Bbuild/gcc -G "Unix Makefiles"
./rebuild.sh
