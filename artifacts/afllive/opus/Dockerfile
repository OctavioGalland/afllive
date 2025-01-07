FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libopus0

# Build and install AFL++
ENV LLVM_CONFIG=llvm-config-14
WORKDIR /opt
COPY afllive /opt/afllive
WORKDIR /opt/afllive/aflplusplus
RUN make -j$(nproc)
RUN make install

# Build autoharness
WORKDIR /opt
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)

# Build function-picker
WORKDIR /opt
WORKDIR /opt/afllive/testamp
RUN sed -i -e 's/-t 10000/-t 999999/g' setup_fuzzers.py
RUN make -j$(nproc)
WORKDIR /opt/afllive/testamp/lib
RUN make libpick.so

# Clone opus
WORKDIR /opt
RUN wget https://archive.mozilla.org/pub/opus/opus-1.3.1.tar.gz
RUN tar -xzf opus-1.3.1.tar.gz

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=1
ENV AFL_IGNORE_ABORT=1
ENV CC=afl-clang-fast

# Build OpenSSL for fuzzing with the manually curated config
WORKDIR /opt/
COPY build_log.sh ./
RUN ./build_log.sh
COPY build_fuzz.sh ./
RUN ./build_fuzz.sh

# Run fuzzer
WORKDIR /opt/
RUN sed -i -e 's/24 \* 60 \* 60/60/g' /opt/afllive/testamp/setup_fuzzers.py
CMD ["sh", "-c", "python3 /opt/afllive/testamp/setup_fuzzers.py > setup.log 2>&1"]

