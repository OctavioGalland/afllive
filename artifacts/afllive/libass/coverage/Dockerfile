FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libass9 ffmpeg

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

# Clone target
WORKDIR /opt
RUN wget https://github.com/libass/libass/releases/download/0.17.1/libass-0.17.1.tar.gz
RUN tar -xzf libass-0.17.1.tar.gz

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

# Build target
WORKDIR /opt/libass-0.17.1
RUN ./configure --enable-shared=no --prefix=/opt/libass
RUN make -j
RUN make install

WORKDIR /opt
RUN wget https://ffmpeg.org/releases/ffmpeg-6.0.tar.xz
RUN tar -xf ffmpeg-6.0.tar.xz
WORKDIR /opt/ffmpeg-6.0
RUN PKG_CONFIG_PATH="/opt/libass/lib/pkgconfig/" ./configure --disable-network --enable-small --cc=afl-clang-fast --cxx=afl-clang-fast++ --enable-libass --disable-doc
RUN make -j

WORKDIR /opt
COPY unsub.mp4 ./
COPY subs.srt ./
COPY run.sh ./
# CMD ["afl-fuzz", "-o", "out", "-t", "99999", "-V", "86400", "--", "./ffmpeg-6.0/ffmpeg", "-threads", "1", "-i", "unsub.mp4", "-vf", "subtitles=subs.srt", "-f", "null", "-"]
CMD ["./run.sh"]
