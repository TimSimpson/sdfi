#!/bin/bash
pushd build/gcc
make all
./count_tests
popd
