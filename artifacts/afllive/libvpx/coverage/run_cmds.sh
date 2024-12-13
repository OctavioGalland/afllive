#!/bin/bash

# python3 ./getcov_inplace.py libvpx $i ./vpxbuild/vpxdec './vpxbuild/vpxdec --codec=vp9 --md5 -S --flipuv stream.ivf' $(pwd)/libvpx_config.json > coverage_data/coverage_libvpx_$i.txt
core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl /opt/vpxbuild/vpxdec '/opt/vpxbuild/vpxdec --codec=vp9 --md5 -S --flipuv stream.ivf' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl libvpx 0 /opt/vpxbuild/vpxdec /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_libvpx_0.txt


