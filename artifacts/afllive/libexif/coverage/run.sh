#!/bin/bash

core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl '/opt/libexif/libexif/.libs/libexif.a' './libexif/contrib/examples/photographer exif-samples/jpg/tests/22-canon_tags.jpg' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl libexif 0 '/opt/libexif/libexif/.libs/libexif.a' /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_libexif_0.txt
