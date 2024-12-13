FROM ubuntu:22.04

RUN apt -y update
RUN apt -y install clang-14 git
RUN apt -y install make autoconf automake zlib1g-dev libbz2-dev liblzma-dev libcurl4-gnutls-dev libssl-dev

# get project
WORKDIR /opt/
RUN git clone https://github.com/samtools/htslib.git htslib

WORKDIR /opt/htslib/
RUN git checkout 1.16
RUN git submodule update --init --recursive

# build project
ENV CC=clang-14
ENV CXX=clang++-14
ENV LD=clang-14
ENV CFLAGS="-fsanitize=fuzzer-no-link,address"
ENV CXXFLAGS="-fsanitize=fuzzer-no-link,address"
ENV LDFLAGS="-fsanitize=fuzzer-no-link,address"

RUN autoreconf -i
RUN ./configure
RUN make -j$(nproc) libhts.a test/fuzz/hts_open_fuzzer.o

# build fuzzer
ENV CFLAGS="-fsanitize=fuzzer,address"
ENV LDFLAGS="-fsanitize=fuzzer,address"
RUN $CC -o /opt/hts_open_fuzzer $CFLAGS test/fuzz/hts_open_fuzzer.o libhts.a -lz -lbz2 -llzma -lcurl -lcrypto -lpthread

# set up fuzzer
WORKDIR /opt
COPY ./run.sh ./
COPY c2.fa 'c1#bounds.sam' /opt/

CMD ["./run.sh"]
