#!/bin/bash
cmake --build build/gcc --config Debug --clean-first
build/gcc/count_tests
