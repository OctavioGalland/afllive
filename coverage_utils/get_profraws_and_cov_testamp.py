import os
import sys
import subprocess
import tempfile
import datetime

_, project, binary, config_path, iteration, fuzzing_folders_root, profraws_folder, nproc = sys.argv

unified_corpus_folder = tempfile.mkdtemp()

def list_files_and_creation_date_ordered(path):
    files = []
    output = subprocess.run(['ls', '-lt', path], stdout = subprocess.PIPE).stdout.decode().strip().split('\n')[1:]
    for line in output:
        line = line.strip()
        if len(line) == 0:
            continue
        fields = line.split(' ')
        filename = fields[-1]
        # ls -lt does not yield enough information to create a datetime, so we use os.path.getmtime as well
        creationtime = datetime.datetime.fromtimestamp(os.path.getmtime(f'{path}/{filename}'))
        files.append((creationtime, filename))
    return list(reversed(files))

def list_fuzzing_campaigns(root):
    return [f'{root}/{i[1]}' for i in list_files_and_creation_date_ordered(root)]

def get_cmdline(campaign_root):
    cmdline = ''
    with open(f'{campaign_root}/default/cmdline', 'r') as f:
        cmdline = f.read().replace('\n', ' ')
    return cmdline

def get_corpus_folder(campaign_root):
    return f'{campaign_root}/default/queue'

def get_campaign_name(campaign_root):
    return campaign_root[campaign_root.rfind('/') + 1:]

fuzzing_campaigns = list_fuzzing_campaigns(fuzzing_folders_root)

fuzzing_pwds = {}
with open(f'{fuzzing_folders_root}/pwds.json', 'r') as f:
    fuzzing_pwds = json.load(f)

# We assume that campaigns took place sequentially, instead we should collect all corpuses and go through them in order
# problem: get_cov_from_profraws reads timestamps for each input from the corpus in a single folder, but we have several corpuses
for fuzzing_campaign_root in fuzzing_campaigns:
    cmdline = get_cmdline(fuzzing_campaign_root)
    corpus_folder = get_corpus_folder(fuzzing_campaign_root)
    name = get_campaign_name(fuzzing_campaign_root)
    campaign_pwd = fuzzing_pwds[name]
    fake_corpus_folder = tempfile.mkdtemp()
    # get_profraws expects a directory with nothing but the input corpus, and it will create similarly
    # named .profraw files in the output folder. In order to avoid collissions between corpus entries
    # across campaigns, we need to add suffixes to the name of each input, but this needs to be done in
    # a dedicated folder and without modifying the original corpus. Hence the fake_corpus_folder symlinks
    # This is horrible, but it allows us to re-use previous scripts without breaking existing setups.
    for (_, filename) in list_files_and_creation_date_ordered(corpus_folder):
        os.symlink(f'{corpus_folder}/{filename}', f'{unified_corpus_folder}/{filename}-{name}')
        os.symlink(f'{corpus_folder}/{filename}', f'{fake_corpus_folder}/{filename}-{name}')
    cmdline = ' '.join(['python3', '/opt/afllive/coverage_utils/get_profraws.py', 'afl']  + [f"'{i}'" for i in [binary, cmdline, fake_corpus_folder, config_path, nproc, profraws_folder]])
    os.system(f'cd {campaign_pwd} && ' + cmdline + ' > /dev/null')
    os.system(f'rm -rf {fake_corpus_folder}')

os.system(' '.join(['python3', '/opt/afllive/coverage_utils/get_cov_from_profraws.py', 'afl'] + [f"'{i}'" for i in [project, iteration, binary, unified_corpus_folder, profraws_folder, nproc]]))

