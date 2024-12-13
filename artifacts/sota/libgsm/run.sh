#!/bin/bash

mkdir -p /opt/out/default/queue
cp /opt/untitled.wav /opt/out/default/queue

# Run fuzzer
mkdir -p /opt/out/default/crashes
cd /opt/out/default/crashes
/opt/fuzzer -fork=1 -max_total_time=60 -ignore_crashes=1 -ignore_ooms=1 -ignore_timeouts=1 /opt/out/default/queue
