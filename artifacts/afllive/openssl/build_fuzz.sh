#!/bin/bash

cd openssl
make distclean
CC=afl-clang-fast CFLAGS="-O2 -fno-omit-frame-pointer" LDFLAGS=$CFLAGS ./Configure no-shared no-module no-threads
make -j$(nproc)
cd ..

