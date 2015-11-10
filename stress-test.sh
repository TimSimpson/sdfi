#!/bin/bash
set -e
pushd build/gcc
make stress-test
./stress-test $@
popd
