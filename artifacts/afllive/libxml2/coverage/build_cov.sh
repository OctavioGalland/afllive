#!/bin/bash

cd libxml2
make distclean
CC=clang-14 CFLAGS="-fprofile-instr-generate -fcoverage-mapping" LDFLAGS=$CFLAGS ./configure --enable-static=yes --enable-shared=no
make -j$(nproc)
mkdir /opt/baseline
LLVM_PROFILE_FILE="/opt/baseline/%m-%p.profraw"  make check -j$(nproc)
llvm-profdata-14 merge -sparse /opt/baseline/*.profraw -o /opt/baseline.profdata
cd ..

