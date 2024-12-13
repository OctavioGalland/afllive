#!/bin/bash

mkdir -p /opt/out/default/queue
cp /opt/eurotext.png /opt/out/default/queue/

mkdir -p /opt/out/default/crashes
cd /opt/out/default/crashes
/opt/leptonica-1.83.0/pix_rotate_shear_fuzzer -fork=1 -ignore_timeouts=1 -ignore_ooms=1 -ignore_crashes=1 -max_total_time=60 /opt/out/default/queue
