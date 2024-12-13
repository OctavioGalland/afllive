FROM ubuntu:22.04

RUN apt update
RUN apt install -y clang-14 git cmake perl yasm make gcc g++ wget
RUN ln -s /usr/bin/clang-14 /usr/bin/clang
RUN ln -s /usr/bin/clang++-14 /usr/bin/clang++

RUN apt-get update
RUN apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev
RUN apt-get install -y lld-14 llvm-14 llvm-14-dev clang-14
RUN apt-get install -y gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev

WORKDIR /opt
RUN git clone https://github.com/octaviogalland/afllive
WORKDIR /opt/afllive/aflplusplus
ENV LLVM_CONFIG=llvm-config-14
RUN make -j$(nproc)
RUN make install

WORKDIR /opt
RUN apt-get install -y libboost-dev
WORKDIR /opt/afllive/instrumentation
RUN make -j$(nproc)
WORKDIR /opt/afllive/libgen
RUN make -j$(nproc)

WORKDIR /opt
RUN wget https://github.com/webmproject/libvpx/archive/refs/tags/v1.12.0.tar.gz
RUN tar -xzf v1.12.0.tar.gz
RUN mv libvpx-1.12.0 libvpx

WORKDIR /opt
COPY config.json stream.ivf ./
ENV AH_CONFIG=/opt/config.json

WORKDIR /opt/build
ENV AFL_USE_ASAN=1
# ENV ASAN_OPTIONS=:handle_segv=1:handle_abort=1:handle_sigfpe=1:fast_unwind_on_fatal=1:allocator_may_return_null=1:detect_stack_use_after_return=1:max_uar_stack_size_log=17:detect_leaks=0

RUN CC=afl-clang-fast CXX=afl-clang-fast++ LD=afl-clang-fast++ CFLAGS='-DVPX_MAX_ALLOCABLE_MEMORY=4294967296' CXXFLAGS='-DVPX_MAX_ALLOCABLE_MEMORY=4294967296' ../libvpx/configure --disable-unit-tests --size-limit=12288x12288 --enable-debug
RUN make -j$(nproc)

WORKDIR /opt

# Remember to give it a generous timeout!
CMD ["afl-fuzz", "-o", "out", "-V", "60", "--", "./build/vpxdec", "--codec=vp9", "--md5", "-S", "--flipuv", "stream.ivf"]

