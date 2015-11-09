#!/bin/bash
set -e
pushd build/gcc
make count_tests
./count_tests
make worker_master_comm
./worker_master_comm
popd
