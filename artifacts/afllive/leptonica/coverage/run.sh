#!/bin/bash

# python3 getcov_inplace.py leptonica 0 './tesseract-5.3.0/tesseract' './tesseract-5.3.0/tesseract /opt/eurotext.png - -l eng' config.json > /opt/coverage_leptonica_0.txt
# wait
core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl '/opt/tesseract-5.3.0/tesseract' '/opt/tesseract-5.3.0/tesseract /opt/eurotext.png - -l eng' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl boringssl 0 '/opt/tesseract-5.3.0/tesseract' /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_leptonica_0.txt
