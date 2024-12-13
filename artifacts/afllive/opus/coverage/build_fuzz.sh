#!/bin/bash

cd opus-1.3.1
make distclean
CC=afl-clang-fast CFLAGS="-fprofile-instr-generate -fcoverage-mapping" LDFLAGS=$CFLAGS ./configure --enable-shared=no
make -j$(nproc)
make check
cd ..

