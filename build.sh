#!/bin/bash
mkdir build
pushd build
cmake ..
popd
./test.sh

