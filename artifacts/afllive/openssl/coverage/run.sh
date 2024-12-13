#!/bin/bash

# mkdir out
# for i in {0..19}
# do
#     python3 getcov_inplace.py openssl $i $(pwd)/openssl/apps/openssl $(pwd)/config.json $(pwd)/results/fuzzing_$i/ $(pwd)/results/pwds.json > out/coverage_openssl_$i.txt
# done

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
# mv /opt/baseline.profdata /opt/profraws_0/baseline.profraw
python3 /opt/afllive/coverage_utils/get_profraws_and_cov_testamp.py openssl /opt/openssl/apps/openssl /opt/config.json 0 /opt/corpuses/out_0 /opt/profraws_0 1 > /opt/coverage_openssl_0.txt

