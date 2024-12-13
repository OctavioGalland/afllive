FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libleptonica-dev tesseract-ocr

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

WORKDIR /opt
RUN wget https://github.com/tesseract-ocr/tesseract/archive/refs/tags/5.3.0.tar.gz
RUN wget https://github.com/DanBloomberg/leptonica/releases/download/1.83.0/leptonica-1.83.0.tar.gz
RUN tar -xzf 5.3.0.tar.gz
RUN tar -xzf leptonica-1.83.0.tar.gz

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=1
ENV TESSDATA_PREFIX=/opt/tesseract-5.3.0/tessdata/
RUN wget https://tesseract-ocr.github.io/tessdoc/images/eurotext.png

WORKDIR /opt/
COPY build.sh ./
RUN ./build.sh afl
COPY ./eng.traineddata ./tesseract-5.3.0/tessdata

# Run fuzzer
CMD ["afl-fuzz", "-o", "out", "-t", "99999", "-V", "60", "--", "./tesseract-5.3.0/tesseract", "/opt/eurotext.png", "-", "-l", "eng"]

