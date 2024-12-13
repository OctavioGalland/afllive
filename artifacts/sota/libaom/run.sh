#!/bin/bash

mkdir -p /opt/out/default/queue
cp /opt/raw.av1 /opt/out/default/queue

mkdir -p /opt/out/default/crashes
cd /opt/out/default/crashes
/opt/fuzzer -ignore_crashes=1 -ignore_ooms=1 -ignore_timeouts=1 -fork=1 -detect_leaks=0 -max_total_time=60 /opt/out/default/queue
