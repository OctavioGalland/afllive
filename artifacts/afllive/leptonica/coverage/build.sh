#!/bin/bash


if [ $1 == "afl" ]
then
    echo "setting up env for AFL"
    export CC=afl-clang-fast
    export CXX=afl-clang-fast++
elif [ $1 == "aflcov" ]
then
    echo "setting up env for AFL"
    export CC=afl-clang-fast
    export CXX=afl-clang-fast++
    export CFLAGS="-fprofile-instr-generate -fcoverage-mapping"
    export CXXFLAGS="$CFLAGS"
    export LDFLAGS="$CFLAGS"

else
    echo "unrecognized target \"$1\""
    echo "available targets are 'picker' and 'afl'"
    exit
fi

rm -rf install > /dev/null 2>&1
mkdir install

cd leptonica-1.83.0
export PICKER_BUILDING_LIB=1
make distclean > /dev/null 2>&1
# ./configure --prefix=$(pwd)/../install
./configure --prefix=$(pwd)/../install --enable-shared=no
make -j$(nproc)
make install
unset PICKER_BUILDING_LIB
cd ..

cd install/lib
ln libleptonica.so liblept.so
ln libleptonica.a liblept.a
cd ../..

cd tesseract-5.3.0
make distclean > /dev/null 2>&1
./autogen.sh
LEPTONICA_LIBS="-L$(pwd)/../install/lib -llept -Wl,-rpath=$(pwd)/../install/lib -lpng -lz -lm -ltiff -lwebp -ljpeg -lgif -lopenjp2" ./configure --enable-debug --enable-shared=no
make -j$(nproc)
cd ..

# echo "If everything went well you should be able to run tesseract with 'TESSDATA_PREFIX=$(pwd)/tesseract/tessdata/ LD_LIBRARY_PATH=$(pwd)/tesseract/.libs ./tesseract/.libs/tesseract ~/Downloads/eurotext.png - -l eng'"
