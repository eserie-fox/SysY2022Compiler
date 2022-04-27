#!/bin/bash
cd build
make Tests
./tests/Tests $@