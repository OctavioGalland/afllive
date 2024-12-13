FROM ubuntu:22.04

RUN apt -y update
RUN apt -y install clang-14 git
RUN apt -y install make autoconf automake libtool pkg-config cmake nasm wget

WORKDIR /opt/
RUN wget https://github.com/DanBloomberg/leptonica/releases/download/1.83.0/leptonica-1.83.0.tar.gz
RUN tar -xzf leptonica-1.83.0.tar.gz

WORKDIR /opt/leptonica-1.83.0
ENV CC=clang-14
ENV CXX=clang++-14
ENV CFLAGS="-fsanitize=address"
ENV CXXFLAGS="-fsanitize=address"
RUN ./configure CC="$CC" CXX="$CXX" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"
RUN make fuzzers CC="$CC" CXX="$CXX" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"

WORKDIR /opt/
COPY run.sh eurotext.png /opt/

CMD ["./run.sh"]
