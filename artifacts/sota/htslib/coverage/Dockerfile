FROM ubuntu:22.04

RUN apt -y update
RUN apt -y install clang-14 git
RUN apt -y install make autoconf automake zlib1g-dev libbz2-dev liblzma-dev libcurl4-gnutls-dev libssl-dev

# get coverage utils
WORKDIR /opt
RUN git clone https://github.com/octaviogalland/afllive

# get project
WORKDIR /opt/
RUN git clone https://github.com/samtools/htslib.git htslib
WORKDIR /opt/htslib/
RUN git checkout 1.16
RUN git submodule update --init --recursive

# build with coverage
WORKDIR /opt/htslib
ENV CC=clang-14
ENV CXX=clang++-14
ENV LD=clang-14
ENV CFLAGS="-fsanitize=fuzzer-no-link,address -fprofile-instr-generate -fcoverage-mapping"
ENV CXXFLAGS="-fsanitize=fuzzer-no-link,address -fprofile-instr-generate -fcoverage-mapping"
ENV LDFLAGS="-fsanitize=fuzzer-no-link,address -fprofile-instr-generate -fcoverage-mapping"
RUN autoreconf -i
RUN ./configure
RUN make -j$(nproc) libhts.a test/fuzz/hts_open_fuzzer.o

# build fuzzer with coverage
ENV CFLAGS="-fsanitize=fuzzer,address -fprofile-instr-generate -fcoverage-mapping"
ENV CXXFLAGS="-fsanitize=fuzzer,address -fprofile-instr-generate -fcoverage-mapping"
ENV LDFLAGS="-fsanitize=fuzzer,address -fprofile-instr-generate -fcoverage-mapping"
RUN $CC -o /opt/hts_open_fuzzer $CFLAGS test/fuzz/hts_open_fuzzer.o libhts.a -lz -lbz2 -llzma -lcurl -lcrypto -lpthread

# get coverage
WORKDIR /opt
COPY run.sh ./
CMD ["./run.sh"]
