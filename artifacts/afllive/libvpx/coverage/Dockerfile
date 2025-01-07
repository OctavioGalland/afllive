FROM ubuntu:22.04

RUN apt update

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

COPY afllive /opt/afllive
WORKDIR /opt/afllive/aflplusplus
ENV LLVM_CONFIG=llvm-config-14
RUN make -j$(nproc)
RUN make install

WORKDIR /opt
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)

WORKDIR /opt

RUN wget https://github.com/webmproject/libvpx/archive/refs/tags/v1.12.0.tar.gz
RUN tar -xzf v1.12.0.tar.gz
RUN mv libvpx-1.12.0 libvpx

WORKDIR /opt
COPY config.json /opt/
ENV AH_CONFIG=/opt/config.json

WORKDIR /opt/vpxbuild

RUN CC=afl-clang-fast CXX=afl-clang-fast++ LD=afl-clang-fast++ CFLAGS='-DVPX_MAX_ALLOCABLE_MEMORY=4294967296 -fprofile-instr-generate -fcoverage-mapping' CXXFLAGS='-DVPX_MAX_ALLOCABLE_MEMORY=4294967296 -fprofile-instr-generate -fcoverage-mapping' LDFLAGS="-fprofile-instr-generate -fcoverage-mapping" ../libvpx/configure --disable-unit-tests --size-limit=12288x12288 --enable-debug
RUN make -j$(nproc)

WORKDIR /opt
COPY stream.ivf run_cmds.sh /opt/
CMD ["./run_cmds.sh"]
