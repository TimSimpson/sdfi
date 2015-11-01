#!/bin/bash
set -e
set -x

echo 'Installing necessary dependencies...'

sudo apt-get install libboost1.58-all-dev
sudo apt-get install cmake

echo 'Finished. Enter /word_max and execute build.sh to create the programs.'
echo 'To generate test files, run ./create_text.sh (found in this directory).'
