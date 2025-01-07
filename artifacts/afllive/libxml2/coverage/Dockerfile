FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget python3-libxml2
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libxml2

# Build and install AFL++
ENV LLVM_CONFIG=llvm-config-14
WORKDIR /opt
COPY afllive /opt/afllive
WORKDIR /opt/afllive/aflplusplus
RUN make -j$(nproc)
RUN make install

# Build autoharness
WORKDIR /opt
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)

# Build function-picker
WORKDIR /opt
WORKDIR /opt/afllive/testamp
RUN make -j$(nproc)
WORKDIR /opt/afllive/testamp/lib
RUN make libpick.so

# Clone OpenSSL and move to release branch
WORKDIR /opt
RUN git clone https://gitlab.gnome.org/GNOME/libxml2
WORKDIR /opt/libxml2
RUN git checkout v2.10.3
RUN ./autogen.sh

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=0
ENV CC=afl-clang-fast

# Build OpenSSL for fuzzing with the manually curated config
WORKDIR /opt/
COPY build_log.sh build_cov.sh build_fuzz.sh run.sh ./
RUN ./build_log.sh
RUN ./build_cov.sh
RUN ./build_fuzz.sh

# Run fuzzer
WORKDIR /opt/
CMD ["./run.sh"]

