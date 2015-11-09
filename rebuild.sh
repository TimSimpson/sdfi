#!/bin/bash
set -e
pushd build/gcc
make all
./count_tests
popd
