FROM ubuntu:22.04

# Add source repo and update
RUN echo 'deb-src http://archive.ubuntu.com/ubuntu/ jammy universe multiverse main restricted' >> /etc/apt/sources.list
RUN apt -y update
RUN apt -y upgrade

# Install dependencies
RUN apt -y install git gcc g++ clang-14 build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev llvm-14-dev llvm-14 lld-14 libboost-dev wget
RUN apt -y install gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
RUN apt -y build-dep libgsm1-dev

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
RUN wget https://www.quut.com/gsm/gsm-1.0.22.tar.gz
RUN tar -xzf gsm-1.0.22.tar.gz
RUN echo "46c46,47\n< CC		= gcc -ansi -pedantic\n---\n> # CC		= gcc -ansi -pedantic\n> CC		= afl-clang-fast" > Makefile.patch
RUN patch gsm-1.0-pl22/Makefile Makefile.patch
RUN git clone https://github.com/openitu/STL
WORKDIR /opt/STL
RUN git checkout v2022
RUN find . -name "gsm_*.c" -delete
RUN echo '4,5c4,7\n< add_executable(rpedemo rpedemo.c add.c code.c debug.c decode.c long_ter.c lpc.c preproce.c rpe.c gsm_dest.c gsm_deco.c gsm_enco.c gsm_expl.c gsm_impl.c gsm_crea.c gsm_prin.c gsm_opti.c rpeltp.c short_te.c table.c ../g711/g711.c)\n< target_link_libraries(rpedemo ${M_LIBRARY})\n---\n> \n> # add_executable(rpedemo rpedemo.c add.c code.c debug.c decode.c long_ter.c lpc.c preproce.c rpe.c gsm_dest.c gsm_deco.c gsm_enco.c gsm_expl.c gsm_impl.c gsm_crea.c gsm_prin.c gsm_opti.c rpeltp.c short_te.c table.c ../g711/g711.c)\n> add_executable(rpedemo rpedemo.c rpeltp.c ../g711/g711.c)\n> target_link_libraries(rpedemo ${M_LIBRARY} -L/opt/gsm-1.0-pl22/lib gsm)\n8c10\n< target_link_libraries(test-add ${M_LIBRARY})\n---\n> target_link_libraries(test-add ${M_LIBRARY} -L/opt/gsm-1.0-pl22/lib gsm)' > CMakeLists.txt.patch
RUN patch src/rpeltp/CMakeLists.txt ./CMakeLists.txt.patch

# Setup env
WORKDIR /opt
COPY config.json ./
ENV AH_CONFIG=/opt/config.json
ENV AFL_USE_ASAN=1

COPY build.sh untitled.wav ./
RUN ./build.sh

# Run fuzzer
WORKDIR /opt
CMD ["afl-fuzz", "-o", "out", "-t", "999999", "-V", "60", "--", "./STL/build/bin/rpedemo", "-u", "untitled.wav", "/dev/null"]
