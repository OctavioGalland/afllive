#!/bin/bash

# python3 getcov_inplace.py htslib 0 samtools/samtools './samtools/samtools view --write-index -C -T ./htslib/test/c2.fa htslib/test/c1#bounds.sam -o /dev/null##idx##/dev/null' config.json > /opt/coverage_htslib_0.txt
core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl /opt/samtools/samtools '/opt/samtools/samtools view --write-index -C -T /opt/htslib/test/c2.fa htslib/test/c1#bounds.sam -o /dev/null##idx##/dev/null' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl htslib 0 /opt/samtools/samtools /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_htslib_0.txt
