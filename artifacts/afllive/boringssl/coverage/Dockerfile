FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y install ninja-build golang libunwind-dev
RUN apt -y build-dep android-libboringssl

# Build and install AFL++
ENV LLVM_CONFIG=llvm-config-14
WORKDIR /opt
RUN git clone https://github.com/octaviogalland/afllive
WORKDIR /opt/afllive/aflplusplus
RUN make -j$(nproc)
RUN make install

# Build autoharness
WORKDIR /opt
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)

# Clone BoringSSL and move to stable branch
WORKDIR /opt
RUN git clone https://boringssl.googlesource.com/boringssl
WORKDIR /opt/boringssl
RUN git checkout cd95210465496ac2337b313cf49f607762abe286

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=0
ENV CC=afl-clang-fast
ENV CXX=afl-clang-fast++
ENV CFLAGS="-fprofile-instr-generate -fcoverage-mapping"
ENV CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping"
ENV LDFLAGS="-fprofile-instr-generate -fcoverage-mapping"

# Build BoringSSL for fuzzing with the manually curated config
WORKDIR /opt/boringssl
RUN cmake -GNinja -B build
RUN ninja -C build

WORKDIR /opt/boringssl
COPY run.sh /opt/
CMD ["/opt/run.sh"]

