#!/bin/bash

core_count=1

rm -rf /opt/profraws_0
mkdir /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_profraws.py afl '/opt/libass/lib/libass.a' './ffmpeg-6.0/ffmpeg -threads 1 -i unsub.mp4 -vf subtitles=subs.srt -f null -' /opt/corpuses/out_0/default/queue /opt/config.json $core_count /opt/profraws_0
python3 /opt/afllive/coverage_utils/get_cov_from_profraws.py afl libass 0 '/opt/ffmpeg-6.0/ffmpeg' /opt/corpuses/out_0/default/queue /opt/profraws_0 $core_count > /opt/coverage_libass_0.txt
