#!/bin/bash

# mkdir out
# for i in {0..19}
# do
#     python3 getcov_inplace.py libxml2 $i '-object /opt/libxml2/runtest' $(pwd)/config.json $(pwd)/results/fuzzing_$i/ $(pwd)/results/pwds.json > out/coverage_libxml2_$i.txt
# done

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws_and_cov_testamp.py libxml2 /opt/libxml2/runtest /opt/config.json 0 /opt/corpuses/out_0 /opt/profraws_0 1 > /opt/coverage_libxml2_0.txt
