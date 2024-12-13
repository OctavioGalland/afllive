export AFL_USE_ASAN=1
export CC=afl-clang-fast

cd gsm-1.0-pl22
make clean
make -j16
cd ..

cd STL
rm -rf build 2> /dev/null
mkdir build
cd build
LDFLAGS="-fprofile-instr-generate -fcoverage-mapping" cmake ..
make -j16
cd ../..
