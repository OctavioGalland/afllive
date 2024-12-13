#!/bin/bash

cd openssl
make distclean
CC=clang-14 CFLAGS="-fpass-plugin=/opt/afllive/testamp/pass/pass.so" LDFLAGS="-L/opt/afllive/testamp/lib" ./Configure no-shared no-module no-threads -lpick
make -j$(nproc)
make HARNESS_JOBS=$(nproc) test
cd ..

