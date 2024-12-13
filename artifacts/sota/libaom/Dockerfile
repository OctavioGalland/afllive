FROM ubuntu:22.04

RUN apt -y update
RUN apt -y install clang-14 git 
RUN apt -y install cmake perl yasm make gcc g++ wget

WORKDIR /opt
RUN git clone https://aomedia.googlesource.com/aom
ENV CC=clang-14
ENV CXX=clang++-14
WORKDIR /opt/aom
RUN git checkout 3613e5d09d6ce09969e72fb8fd3cc811609442d4

WORKDIR /opt/build
ENV EXTRA_C_FLAGS="-UNDEBUG -DDO_RANGE_CHECK_CLAMP=1 -DAOM_MAX_ALLOCABLE_MEMORY=1073741824"
ENV ASAN_OPTIONS=:handle_segv=1:handle_abort=1:handle_sigfpe=1:fast_unwind_on_fatal=1:allocator_may_return_null=1:detect_stack_use_after_return=1:max_uar_stack_size_log=17:detect_leaks=0

RUN CFLAGS="-fsanitize=address,fuzzer-no-link" CXXFLAGS="-fsanitize=address,fuzzer-no-link" cmake ../aom -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONFIG_PIC=1 -DFORCE_HIGHBITDEPTH_DECODING=0 -DENABLE_EXAMPLES=0 -DENABLE_DOCS=0 -DENABLE_TESTS=0 -DCONFIG_SIZE_LIMIT=1 -DDECODE_HEIGHT_LIMIT=12288 -DDECODE_WIDTH_LIMIT=12288 -DAOM_EXTRA_C_FLAGS="${EXTRA_C_FLAGS}" -DAOM_EXTRA_CXX_FLAGS="${EXTRA_C_FLAGS}"
# RUN CFLAGS="-fsanitize=address,fuzzer-no-link" CXXFLAGS="-fsanitize=address,fuzzer-no-link" cmake ../aom
RUN make -j$(nproc)

WORKDIR /opt
RUN wget https://github.com/HexHive/FuzzGen/raw/master/examples/libaom/av1_dec_fuzzer-LLVMFuzzerTestOneInput%2Bdecode_to_md5-main%2Bdecode_with_drops-main%2Blossless_encoder-main%2Bsimple_decoder-main/av1_dec_fuzzer-LLVMFuzzerTestOneInput%2Bdecode_to_md5-main%2Bdecode_with_drops-main%2Blossless_encoder-main%2Bsimple_decoder-main_fuzzer.cpp --output-document=harness.cpp
RUN clang++-14 -fsanitize=address,fuzzer -Iaom -Ibuild harness.cpp build/libaom.a -o fuzzer

ENV ASAN_OPTIONS=detect_leaks=0

WORKDIR /opt/
COPY run.sh raw.av1 ./

CMD ["./run.sh"]

