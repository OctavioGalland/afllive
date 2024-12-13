#!/bin/bash

mkdir /opt/profraws_0

python3 /opt/afllive/coverage_utils/get_profraws.py libfuzzer /opt/leptonica-1.83.0/pix_rotate_shear_fuzzer /opt/leptonica-1.83.0/pix_rotate_shear_fuzzer /opt/corpuses/out_0/default/queue 1 /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py libfuzzer leptonica 0 /opt/leptonica-1.83.0/pix_rotate_shear_fuzzer /opt/corpuses/out_0/default/queue/ /opt/profraws_0/ 1 > /opt/coverage_leptonica_0.txt
