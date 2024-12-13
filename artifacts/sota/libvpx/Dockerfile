FROM ubuntu:22.04

RUN apt -y update
RUN apt install -y clang-14 git cmake perl yasm make gcc g++ wget
RUN ln -s /usr/bin/clang-14 /usr/bin/clang
RUN ln -s /usr/bin/clang++-14 /usr/bin/clang++

WORKDIR /opt
RUN wget https://github.com/webmproject/libvpx/archive/refs/tags/v1.12.0.tar.gz
RUN tar -xzf v1.12.0.tar.gz
RUN mv libvpx-1.12.0 libvpx

WORKDIR /opt
RUN wget https://github.com/HexHive/FuzzGen/raw/master/examples/libvpx/simple_decoder-main%2Bvp8cx_set_ref-main%2Bvp9_lossless_encoder-main%2Bvpx_dec_fuzzer_2-LLVMFuzzerTestOneInput/simple_decoder-main%2Bvp8cx_set_ref-main%2Bvp9_lossless_encoder-main%2Bvpx_dec_fuzzer_2-LLVMFuzzerTestOneInput_fuzzer.cpp --output-document=harness.cpp
RUN echo "415c415,416\n<         struct vpx_codec_ctx *ctx_hEP_1;// = &ctx_hEP_0;\n---\n>         struct vpx_codec_ctx dummy;// = &ctx_hEP_0;\n>         struct vpx_codec_ctx *ctx_hEP_1 = &dummy;// = &ctx_hEP_0;\n" > fix.patch
RUN patch harness.cpp fix.patch

WORKDIR /opt/build

ENV ASAN_OPTIONS=:handle_segv=1:handle_abort=1:handle_sigfpe=1:fast_unwind_on_fatal=1:allocator_may_return_null=1:detect_stack_use_after_return=1:max_uar_stack_size_log=17:detect_leaks=0

RUN CC=clang-14 CXX=clang++-14 LD=clang++-14 CFLAGS='-fsanitize=address,fuzzer-no-link -DVPX_MAX_ALLOCABLE_MEMORY=4294967296' CXXFLAGS='-fsanitize=address,fuzzer-no-link -DVPX_MAX_ALLOCABLE_MEMORY=4294967296' LDFLAGS='-fsanitize=address,fuzzer-no-link -DVPX_MAX_ALLOCABLE_MEMORY=4294967296' ../libvpx/configure --disable-unit-tests --size-limit=12288x12288 --enable-debug --disable-examples
RUN make -j$(nproc)

WORKDIR /opt/
RUN clang++-14 -fsanitize=address,fuzzer -Ilibvpx -Ibuild harness.cpp -o fuzzer -Lbuild -lvpx -g

WORKDIR /opt
COPY run.sh stream.ivf ./

CMD ["./run.sh"]

