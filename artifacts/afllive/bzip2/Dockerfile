FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep bzip2

# Build and install AFL++
ENV LLVM_CONFIG=llvm-config-14
WORKDIR /opt
COPY afllive /opt/afllive
WORKDIR /opt/afllive/aflplusplus
RUN make -j$(nproc)
RUN make install

# Build autoharness
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)

# Clone bzip2
WORKDIR /opt
RUN wget https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz
RUN tar -xzf bzip2-1.0.8.tar.gz

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=1
ENV CC=afl-clang-fast
ENV CXX=afl-clang-fast++

# Build BoringSSL for fuzzing with the manually curated config
WORKDIR /opt/bzip2-1.0.8
RUN sed -i 's/^CC=gcc$/CC=afl-clang-fast/g' Makefile
RUN make

WORKDIR /opt
COPY sample.bz2 ./
CMD ["afl-fuzz", "-V", "60", "-o", "out", "-t", "100000", "--", "./bzip2-1.0.8/bzip2", "-d", "-c", "sample.bz2"]

