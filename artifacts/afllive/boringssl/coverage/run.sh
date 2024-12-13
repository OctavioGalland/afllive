#!/bin/bash

core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl /opt/boringssl/build/crypto/crypto_test /opt/boringssl/build/crypto/crypto_test /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl boringssl 0 /opt/boringssl/build/crypto/crypto_test /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_boringssl_0.txt
