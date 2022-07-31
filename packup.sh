#!/bin/bash
mkdir -p collection

cp source collection/ -r
cp library collection/ -r
mkdir -p build
cd build
cmake ..
make
cd ..
cp build/library/parser collection/library/ -r