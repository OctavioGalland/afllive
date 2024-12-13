export AFL_USE_ASAN=1
export CC=afl-clang-fast

core_count=8

cd gsm-1.0-pl22
make clean
make -j$core_count
cd ..

cd STL
rm -rf build 2> /dev/null
mkdir build
cd build
cmake ..
make -j$core_count
cd ../..
