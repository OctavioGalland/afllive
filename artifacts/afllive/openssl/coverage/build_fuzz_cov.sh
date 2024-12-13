#!/bin/bash

cd openssl
make distclean
CC=afl-clang-fast CFLAGS="-g -O0 -fno-omit-frame-pointer -fprofile-instr-generate -fcoverage-mapping" LDFLAGS=$CFLAGS ./Configure no-shared no-module no-threads
make -j$(nproc)
cd ..

