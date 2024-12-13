#!/bin/bash


echo "Setting core_pattern to 'core'..."
echo core | sudo tee /proc/sys/kernel/core_pattern

echo "Setting CPU frequency scaling governor to 'performance'..."
cd /sys/devices/system/cpu && (echo performance | sudo tee cpu*/cpufreq/scaling_governor)

echo "Running 'sysctl vm.mmap_rnd_bits=28'..."
echo "(This is required to work around a known bug with old versions of LLVM on newer kernels, see: https://github.com/google/sanitizers/issues/1614)"
sudo sysctl vm.mmap_rnd_bits=28
