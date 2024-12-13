FROM ubuntu:22.04

RUN apt -y update
RUN apt -y install clang-14 git
RUN apt -y install make autoconf automake libtool pkg-config cmake nasm wget

# get coverage utils
WORKDIR /opt
RUN git clone https://github.com/octaviogalland/afllive

# get project
WORKDIR /opt/
RUN wget https://github.com/DanBloomberg/leptonica/releases/download/1.83.0/leptonica-1.83.0.tar.gz
RUN tar -xzf leptonica-1.83.0.tar.gz

# build with coverage
WORKDIR /opt/leptonica-1.83.0
ENV CC=clang-14
ENV CXX=clang++-14
ENV CFLAGS="-fprofile-instr-generate -fcoverage-mapping"
ENV CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping"
RUN ./configure CC=clang-14 CXX=clang++-14 CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"
RUN make fuzzers CC=clang-14 CXX=clang++-14 CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"

# get coverage
WORKDIR /opt/

# work around coverage collection issue
RUN sed -i "s/'leptonica':/#'leptonica':/g" afllive/coverage_utils/get_cov_from_profraws.py

COPY run.sh ./
CMD ["./run.sh"]
