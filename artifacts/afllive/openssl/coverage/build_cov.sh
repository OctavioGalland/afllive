#!/bin/bash

cd openssl
make distclean
CC=clang-14 CFLAGS="-fprofile-instr-generate -fcoverage-mapping" LDFLAGS=$CFLAGS ./Configure no-shared no-module no-threads
make -j$(nproc)
mkdir /opt/baseline
LLVM_PROFILE_FILE="/opt/baseline/%m-%p.profraw" make test
llvm-profdata-14 merge -sparse /opt/baseline/*.profraw -o /opt/baseline.profdata
rm -rf /opt/baseline
cd ..
