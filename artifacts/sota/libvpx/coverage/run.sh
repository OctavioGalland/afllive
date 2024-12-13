#!/bin/bash

mkdir /opt/profraws_0

python3 /opt/afllive/coverage_utils/get_profraws.py libfuzzer /opt/fuzzer /opt/fuzzer /opt/corpuses/out_0/default/queue 1 /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py libfuzzer libvpx 0 /opt/fuzzer /opt/corpuses/out_0/default/queue/ /opt/profraws_0/ 1 > /opt/coverage_libvpx_0.txt

