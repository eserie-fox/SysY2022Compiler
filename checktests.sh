#!/bin/bash
mkdir -p build
cd build
make Tests
cd tests
if [ ! -f "test.txt" ]; then
    cp ../../tests/compile_test_cases/2022_functional/26_while_test1.sy ./test.txt
fi
./Tests $@