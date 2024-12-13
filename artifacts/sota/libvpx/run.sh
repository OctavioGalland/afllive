#!/bin/bash

mkdir -p /opt/out/default/queue
cp ./stream.ivf /opt/out/default/queue/

mkdir -p /opt/out/default/crashes
cd /opt/out/default/crashes
/opt/fuzzer -fork=1 -detect_leaks=0 -max_total_time=60 -ignore_crashes=1 -ignore_ooms=1 -ignore_timeouts=1 /opt/out/default/queue

