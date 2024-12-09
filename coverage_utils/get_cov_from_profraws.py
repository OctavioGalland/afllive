import os
import sys
import subprocess
import datetime
import signal
import tempfile
import math
import json
from time import localtime

_, mode, project, iteration, binary, corpus_folder, profraw_folder, nproc = sys.argv

output_folder = tempfile.mkdtemp()

def muted_system(cmd):
    return os.system(f'{cmd} > /dev/null 2>&1')

def list_files_and_creation_date_ordered(path):
    files = []
    output = subprocess.run(['ls', '-lt', path], stdout = subprocess.PIPE).stdout.decode().strip().split('\n')[1:]
    for line in output:
        line = line.strip()
        if len(line) == 0:
            continue
        fields = line.split()
        filename = fields[8]
        # ls -lt does not yield enough information to create a datetime, so we use os.path.getmtime as well
        creationtime = datetime.datetime.fromtimestamp(os.path.getmtime(f'{path}/{filename}'))
        files.append((creationtime, filename))
    return list(reversed(files))

def list_files(directory):
    # https://stackoverflow.com/a/3207973
    return [f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]

skipPred = {
    'freetype2': lambda l, i: False,
    'libxml2': lambda l, i: i == 30,
    'openssl': lambda l, i: 'apps/' in l,
    'opus': lambda l, i: 'opus_demo.c' in l,
    'zlib': lambda l, i: 'test/example.c' in l,
    'libaom': lambda l, i: any([keyword in l for keyword in ['third_party', 'y4menc.c', 'webmdec.cc', 'video_reader.c', 'tools_common.c', 'rawenc.c', 'obudec.c', 'md5_utils.c', 'ivfdec.c', 'av1_config.c', 'args.c']]),
    'libvpx': lambda l, i: any([keyword in l for keyword in ['third_party', 'vpxdec.c', 'webmdec.cc', 'y4menc.c', 'y4minput.c', 'args.c', 'ivfdec.c', 'md5_utils.c', 'tools_common.c']]),
    'boringssl': lambda l, i: ('build' in l) or ('third_party' in l) or ('fuzz' in l) or ('test' in l),
    'bzip2': lambda l, i: 'bzip2_decompress_target' in l,
    'libass': lambda l, i: ('libass' not in l) or ('fuzz' in l),
    'leptonica': lambda l, i: 'leptonica-1.83.0' not in l,
}

def covreport2linecount(report):
    lineCount = 0
    idx = 0
    totalCount = 0
    for line in report.split('\n'):
        stats = line.split()
        if len(stats) == 13 and stats[0] != 'TOTAL' and ((project not in skipPred) or (not skipPred[project](line, idx))):
            lines = int(stats[7])
            missed = int(stats[8])
            cov = lines - missed
            totalCount += lines
            lineCount += cov
        idx += 1
    return lineCount, totalCount


def get_coverage(files):
    combined_profile_filename = f'/tmp/coverage_{project}_{iteration}_combined.profdata'
    proffileslist = ' '.join([f'{profraw_folder}/{file}' for file in files])
    subprocess.run((f'llvm-profdata-14 merge --num-threads={nproc} -sparse ' + (f'{profraw_folder}/baseline.profraw' if mode == 'afl' else '') + f' {proffileslist} -o {combined_profile_filename}').split(' '))
    report_cmd = f'llvm-cov-14 report {binary} -instr-profile={combined_profile_filename}'
    output = subprocess.run(report_cmd.split(' '), stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout.decode('utf-8')
    return covreport2linecount(output)

def report_cov(cov, windowIdx):
    executed = cov[0]
    total = cov[1]
    print(f'{project}_iter_{iteration}@{windowIdx}: {executed}/{total}')

files = list_files_and_creation_date_ordered(corpus_folder)
profraw_files = []
windowStart = files[0][0]
windowSize = 10

for window in range(math.ceil(24 * 60 / windowSize)):
    consumed = 0
    addedInput = False
    for (creationtime, inputfile) in files:
        if creationtime > windowStart + datetime.timedelta(minutes=windowSize):
            break
        profraw_files.append(f'{inputfile}.profraw')
        consumed += 1
        addedInput = True
    files = files[consumed:]
    if addedInput:
        report_cov(get_coverage(profraw_files), window)
    windowStart += datetime.timedelta(minutes=windowSize)

os.system(f'rm -rf {output_folder}')

