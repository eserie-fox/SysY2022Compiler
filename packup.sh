#!/bin/bash
mkdir -p collection

cp source collection/ -r
cp library collection/ -r
mkdir -p build
cd build
cmake ..
make
cd ..
rm collection/library/parser -rf
cp build/library/parser collection/library/ -r