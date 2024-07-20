import sys
import os
import math
import tempfile
import subprocess
import datetime
import signal
import json
from threading import Thread

mode = sys.argv[1]

if mode == 'afl':
    _, mode, binary, cmdline, corpus_folder, config_filepath, nproc, output_folder = sys.argv
elif mode == 'libfuzzer':
    _, mode, binary, cmdline, corpus_folder, nproc, output_folder = sys.argv
nproc = int(nproc)

files = [f for f in os.listdir(corpus_folder) if os.path.isfile(os.path.join(corpus_folder, f))]
files = [f for f in files if f[0] != '.']

batch_size = math.ceil(len(files)/nproc)

batches = [files[i*batch_size:i*batch_size + batch_size] for i in range(nproc)]

def muted_system(cmd):
    return os.system(f'{cmd} > /dev/null 2>&1')

def get_profraw(file, tmp_configpath):
    proffile = f'{output_folder}/{file}.profraw'
    if mode == 'afl':
        config = {}
        with open(config_filepath, 'r') as f:
            config = json.load(f)
        function_target_name = file[:file.find('-')]
        config["fuzz_target"] = function_target_name
        with open(tmp_configpath, 'w') as f:
            json.dump(config, f, indent=4)
        muted_system(f'AFL_DISABLE_LLVM_INSTRUMENTATION=1 AH_CONFIG="{tmp_configpath}" LLVM_PROFILE_FILE="{proffile}" timeout -k 1 100 {cmdline} < {corpus_folder}/{file}')
        if not os.path.isfile(proffile):
            print(f'{proffile} not created, stubbing!')
            os.system(f'cp {output_folder}/baseline.profraw {proffile}')
    elif mode == 'libfuzzer':
        muted_system(f'LLVM_PROFILE_FILE="{proffile}" timeout -k 1 100 {cmdline} {corpus_folder}/{file}')


# def get_profraw(file):
#     proffile = f'{output_folder}/{file}.profraw'
#     print(f'LLVM_PROFILE_FILE="{proffile}" timeout 100 {cmdline} < {corpus_folder}/{file}')
#     # muted_system(f'LLVM_PROFILE_FILE="{proffile}" timeout 100 {cmdline} < {corpus_folder}/{file}')

def process_batch(batch_no):
    batch = batches[batch_no]
    for file in batch:
        get_profraw(file, f'{output_folder}/config_{batch_no}.json')

if mode == 'afl':
    muted_system(f'AFL_DISABLE_LLVM_INSTRUMENTATION=1 LLVM_PROFILE_FILE="{output_folder}/baseline.profraw" timeout -k 1 180 {cmdline}')
threads = []

for i in range(nproc):
    t = Thread(target=process_batch, args=(i,))
    threads.append(t)
[t.start() for t in threads]
[t.join() for t in threads]

print(f'Done! Prof files stored in {output_folder}!')
