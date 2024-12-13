FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libhts-dev

# Build and install AFL++
ENV LLVM_CONFIG=llvm-config-14
WORKDIR /opt
RUN git clone https://github.com/octaviogalland/afllive
WORKDIR /opt/afllive/aflplusplus
RUN make -j$(nproc)
RUN make install

# Build autoharness
WORKDIR /opt
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)

WORKDIR /opt
RUN git clone https://github.com/samtools/htslib
WORKDIR /opt/htslib
RUN git checkout 1.16
RUN git submodule update --init --recursive

WORKDIR /opt
RUN git clone https://github.com/samtools/samtools
WORKDIR /opt/samtools
RUN git checkout 1.16.1

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=1

WORKDIR /opt/htslib
RUN autoreconf -i
RUN CC=afl-clang-fast ./configure
RUN make -j$(nproc)

WORKDIR /opt/samtools
RUN autoheader
RUN autoconf -Wno-syntax
RUN CC=afl-clang-fast CFLAGS="-I/opt/htslib" LDFLAGS="-L/opt/htslib" ./configure
RUN make -j$(nproc)

# Run fuzzer
WORKDIR /opt
CMD ["afl-fuzz", "-o", "out", "-t", "999999", "-V", "60", "--", "./samtools/samtools", "view", "--write-index", "-C", "-T", "./htslib/test/c2.fa", "htslib/test/c1#bounds.sam", "-o", "/dev/null##idx##/dev/null"]
