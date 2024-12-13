FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libexif12

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

# Clone target
WORKDIR /opt
RUN git clone https://github.com/ianare/exif-samples
RUN git clone https://github.com/libexif/libexif
WORKDIR /opt/libexif
RUN git checkout libexif-0_6_24-release

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=1
ENV CC=afl-clang-fast
ENV CXX=afl-clang-fast++

# Build target
WORKDIR /opt/libexif
RUN autoreconf -i
RUN ./configure --enable-shared=no --enable-static=yes
RUN make
WORKDIR /opt/libexif/contrib/examples
RUN make photographer

WORKDIR /opt
CMD ["afl-fuzz", "-V", "60", "-o", "out", "--", "./libexif/contrib/examples/photographer", "exif-samples/jpg/tests/22-canon_tags.jpg"]
