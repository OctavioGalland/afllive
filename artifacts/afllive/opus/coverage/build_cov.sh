#!/bin/bash

cd opus-1.3.1
make distclean
CC=clang-14 CFLAGS="-fprofile-instr-generate -fcoverage-mapping" LDFLAGS=$CFLAGS ./configure --enable-shared=no
make -j$(nproc)
mkdir /opt/baseline
LLVM_PROFILE_FILE="/opt/baseline/%m-%p.profraw" make check
llvm-profdata-14 merge -sparse /opt/baseline/*.profraw -o /opt/baseline.profdata
rm -rf /opt/baseline
cd ..

