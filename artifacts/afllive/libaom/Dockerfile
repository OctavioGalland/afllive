FROM ubuntu:22.04

RUN apt -y update

# Clang
RUN apt-get install -y clang-14
# Project deps
RUN apt-get install -y git cmake perl yasm make gcc g++ wget

RUN ln -s /usr/bin/clang-14 /usr/bin/clang
RUN ln -s /usr/bin/clang++-14 /usr/bin/clang++

# AFL++ deps
RUN apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev
RUN apt-get install -y lld-14 llvm-14 llvm-14-dev clang-14
RUN apt-get install -y gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev

# Autoharness deps
RUN apt-get install -y libboost-dev

WORKDIR /opt
RUN git clone https://github.com/octaviogalland/afllive
WORKDIR /opt/afllive/aflplusplus
RUN git pull
ENV LLVM_CONFIG=llvm-config-14
RUN make -j$(nproc)
RUN make install

WORKDIR /opt
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)

WORKDIR /opt
RUN git clone https://aomedia.googlesource.com/aom
ENV CC=afl-clang-fast
ENV CXX=afl-clang-fast++
WORKDIR /opt/aom
RUN git checkout 3613e5d09d6ce09969e72fb8fd3cc811609442d4

WORKDIR /opt
COPY raw.av1 config.json ./
ENV AH_CONFIG=/opt/config.json

WORKDIR /opt/build
ENV AFL_USE_ASAN=1
ENV EXTRA_C_FLAGS="-UNDEBUG -DDO_RANGE_CHECK_CLAMP=1 -DAOM_MAX_ALLOCABLE_MEMORY=1073741824"

RUN cmake ../aom -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONFIG_PIC=1 -DFORCE_HIGHBITDEPTH_DECODING=0 -DENABLE_EXAMPLES=1 -DENABLE_DOCS=0 -DENABLE_TESTS=0 -DCONFIG_SIZE_LIMIT=1 -DDECODE_HEIGHT_LIMIT=12288 -DDECODE_WIDTH_LIMIT=12288 -DAOM_EXTRA_C_FLAGS="${EXTRA_C_FLAGS}" -DAOM_EXTRA_CXX_FLAGS="${EXTRA_C_FLAGS}"
RUN make -j$(nproc)

WORKDIR /opt

CMD ["afl-fuzz", "-o", "out", "-V", "60", "--", "./build/aomdec", "--md5", "./raw.av1"]

