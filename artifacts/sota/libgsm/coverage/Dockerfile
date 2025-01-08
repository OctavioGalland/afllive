FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libgsm1-dev

WORKDIR /opt
COPY afllive /opt/afllive

WORKDIR /opt
RUN wget https://www.quut.com/gsm/gsm-1.0.22.tar.gz
RUN tar -xzf gsm-1.0.22.tar.gz
RUN echo '46,47c46,49\n< CC		= gcc -ansi -pedantic\n< CCFLAGS 	= -c -O2 -DNeedFunctionPrototypes=1 -Wall -Wno-comment\n---\n> # CC		= gcc -ansi -pedantic\n> CC = clang-14\n> CCFLAGS 	= -c -O2 -DNeedFunctionPrototypes=1 -Wall -Wno-comment -fsanitize=address,fuzzer-no-link -fprofile-instr-generate -fcoverage-mapping\n> # CCFLAGS 	= -c -O2 -DNeedFunctionPrototypes=1 -Wall -Wno-comment -fprofile-instr-generate -fcoverage-mapping -fsanitize=address,fuzzer-no-link\n52a55,56\n> LDFLAGS = -fsanitize=address,fuzzer-no-link -fprofile-instr-generate -fcoverage-mapping\n> # LDFLAGS = -fprofile-instr-generate -fcoverage-mapping -fsanitize=address,fuzzer-no-link' > Makefile.patch
RUN patch gsm-1.0-pl22/Makefile Makefile.patch
WORKDIR /opt/gsm-1.0-pl22
RUN make -j$(nproc)

WORKDIR /opt
RUN wget https://raw.githubusercontent.com/HexHive/FuzzGen/master/examples/libgsm/cod2lin-main/cod2lin-main_fuzzer.cpp
RUN echo "52a53\n> #include <cstdint>\n285c286\n<     buflen = (size - ninp) / nbufs - 1;             // find length of each buffer\n---\n>     // buflen = (size - ninp) / nbufs - 1;             // find length of each buffer" > cod2lin.patch
RUN patch cod2lin-main_fuzzer.cpp cod2lin.patch
RUN clang++-14 -fprofile-instr-generate -fcoverage-mapping -fsanitize=fuzzer,address -Igsm-1.0-pl22/inc cod2lin-main_fuzzer.cpp gsm-1.0-pl22/lib/libgsm.a -o fuzzer

COPY run.sh ./

# Run fuzzer
CMD ["./run.sh"]
