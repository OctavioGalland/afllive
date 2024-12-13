#!/bin/bash

mkdir -p /opt/out/default/queue
cp /opt/c2.fa /opt/out/default/queue/
cp '/opt/c1#bounds.sam' /opt/out/default/queue/

mkdir -p /opt/out/default/crashes
cd /opt/out/default/crashes
/opt/hts_open_fuzzer -fork=1 -ignore_timeouts=1 -ignore_ooms=1 -ignore_crashes=1 -max_total_time=60 /opt/out/default/queue
