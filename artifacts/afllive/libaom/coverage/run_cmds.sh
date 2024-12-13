#!/bin/bash


# python3 ./getcov_inplace.py libaom 0 ./aombuild/aomdec './aombuild/aomdec --md5 ./raw.av1' $(pwd)/libaom_config.json > /opt/coverage_libaom_0.txt
core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl /opt/aombuild/aomdec '/opt/aombuild/aomdec --md5 ./raw.av1' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl libaom 0 /opt/aombuild/aomdec /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_libaom_0.txt
