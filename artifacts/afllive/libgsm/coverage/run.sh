#!/bin/bash

# python3 getcov_inplace.py libgsm $i STL/build/bin/rpedemo './STL/build/bin/rpedemo -u untitled.wav /dev/null' config.json > results/coverage_libgsm_$i.txt

core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl '/opt/STL/build/bin/rpedemo' '/opt/STL/build/bin/rpedemo -u untitled.wav /dev/null' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl libgsm 0 '/opt/STL/build/bin/rpedemo' /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_libgsm_0.txt

