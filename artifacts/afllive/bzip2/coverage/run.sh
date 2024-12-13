#!/bin/bash

core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl '/opt/bzip2-1.0.8/bzip2' '/opt/bzip2-1.0.8/bzip2 -d -c /opt/sample.bz2' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl bzip2 0 '/opt/bzip2-1.0.8/bzip2' /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_bzip2_0.txt
