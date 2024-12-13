#!/bin/bash

cd opus-1.3.1
make distclean
CC=clang-14 CFLAGS="-fpass-plugin=/opt/afllive/testamp/pass/pass.so" LDFLAGS="-L/opt/afllive/testamp/lib -Wl,-rpath=/opt/afllive/testamp/lib -lpick" ./configure --enable-shared=no
make -j$(nproc)
make check
cd ..

