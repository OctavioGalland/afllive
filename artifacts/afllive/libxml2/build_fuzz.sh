#!/bin/bash

cd libxml2
make distclean
CC=afl-clang-fast CFLAGS="-g -O0 -fno-omit-frame-pointer" LDFLAGS=$CFLAGS ./configure --enable-static=yes --enable-shared=no
make -j$(nproc)
make check -j$(nproc)
cd ..

