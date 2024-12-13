#!/bin/bash

cd opus-1.3.1
make distclean
CC=afl-clang-fast ./configure --enable-shared=no
make -j$(nproc)
make check
cd ..

