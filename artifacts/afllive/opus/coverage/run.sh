#!/bin/bash

# mkdir out
# for i in {0..19}
# do
#     python3 getcov_inplace.py opus $i '/opt/opus-1.3.1/tests/test_opus_encode' $(pwd)/config.json $(pwd)/results/fuzzing_$i/ $(pwd)/results/pwds.json > out/coverage_opus_$i.txt
# done

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
# mv /opt/baseline.profdata /opt/profraws_0/baseline.profraw
python3 /opt/afllive/coverage_utils/get_profraws_and_cov_testamp.py opus /opt/opus-1.3.1/tests/test_opus_encode /opt/config.json 0 /opt/corpuses/out_0 /opt/profraws_0 1 > /opt/coverage_opus_0.txt
