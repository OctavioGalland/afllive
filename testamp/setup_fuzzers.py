#!/usr/bin/python3

import os
import sys
import time
import json
import math

FUN_STATE = 'fun'
ENV_STATE = 'env'
CMD_STATE = 'cmd'
CWD_STATE = 'cwd'

targets = []
with open(os.environ["AH_CONFIG"], "r") as f:
    data = json.load(f)
    targets = data["targets"]
print(targets)
executed_targets = set()

def isTargetInSignature(target, signature):
    # case where the user specified the whole module name
    if f' {target} ' in signature:
        return True
    # case where the user only specified the function name
    if f':{target} ' in signature:
        return True
    return False

fuzzing_pwds = {}

def get_fuzzing_commands():
    fuzzingCommands = []
    state = ''
    fuzzingCommand = ''
    targetsToFuzz = set()
    i = 0
    with open('/opt/fuzz.log', 'r') as f:
        for line in f:
            line = line.strip()
            if len(line) == 0:
                continue
            if 'FUNCTIONS:' in line:
                state = FUN_STATE
                if len(targetsToFuzz) > 0:
                    fuzzingCommands.append((targetsToFuzz, fuzzingCommand))
                    i += 1
                fuzzingCommand = ''
                targetsToFuzz = set()
            elif 'ENVIRONMENT:' in line:
                state = ENV_STATE
            elif 'CMDLINE:' in line:
                state = CMD_STATE
            elif 'CWD:' in line:
                state = CWD_STATE
            else:
                if state == FUN_STATE:
                    useful = False
                    for target in targets:
                        if isTargetInSignature(target, line):
                            targetsToFuzz.add(target)
                            global executed_targets
                            executed_targets.add(target)
                elif state == ENV_STATE:
                    var = line[0:line.find('=')]
                    val = line[line.find('=') + 1:]
                    fuzzingCommand = f"{var}='{val}' " + fuzzingCommand
                elif state == CMD_STATE:
                    fuzzingCommand = fuzzingCommand + f' afl-fuzz -o /opt/fuzzing/fuzzer_{i} -t 10000 -- ' + line + f'  '
                elif state == CWD_STATE:
                    fuzzingCommand = 'cd ' + line + ' && ' + fuzzingCommand
                    fuzzing_pwds[f'fuzzer_{i}'] = line
        fuzzingCommands.append((targetsToFuzz, fuzzingCommand))
    return fuzzingCommands

os.system('rm -rf /opt/fuzzing/*')
os.system('mkdir /opt/fuzzing')
print('Collecting fuzzing info')
fuzzing_commands = get_fuzzing_commands()

with open(f'/opt/fuzzing/pwds.json', 'w') as f:
    f.write(json.dumps(fuzzing_pwds, indent=4))

print('Starting fuzzers')
fuzzed_targets = set()
while len(fuzzed_targets) < len(executed_targets):
    cmd_bl = []
    best_fuzz_cmd = None
    best_fuzz_cmd_targets = set()
    for fuzz_cmd in fuzzing_commands:
        if fuzz_cmd[1] in cmd_bl:
            continue
        fuzz_cmd_targets = fuzz_cmd[0]
        fuzz_cmd_unfuzzed_targets = fuzz_cmd_targets.difference(fuzzed_targets)
        if len(fuzz_cmd_unfuzzed_targets) > len(best_fuzz_cmd_targets):
            best_fuzz_cmd_targets = fuzz_cmd_unfuzzed_targets
            best_fuzz_cmd = fuzz_cmd
    if len(best_fuzz_cmd_targets) > 0:
        fuzzing_commands.remove(best_fuzz_cmd)
        cmd = best_fuzz_cmd[1]
        fuzzing_quota = math.ceil(24 * 60 * 60 * len(best_fuzz_cmd_targets) / len(executed_targets))
        cmd = cmd.replace(' afl-fuzz ', 'AH_BLACKLIST=\':' + ':'.join(fuzzed_targets) + ':\' afl-fuzz ')
        cmd = cmd.replace('afl-fuzz ', f'afl-fuzz -V {fuzzing_quota} ')
        if 'ASAN_OPTIONS=' in cmd:
            cmd = cmd.replace('ASAN_OPTIONS=\'', 'ASAN_OPTIONS=\'symbolize=0:abort_on_error=1:')
        targets = best_fuzz_cmd[0].difference(fuzzed_targets)
        print (f'Running {cmd}\n\n\n')
        print (f'Which should fuzz {targets}\n\n\n')
        status = os.system(cmd)
        if os.WEXITSTATUS(status) == 0 or os.WTERMSIG(status) == 2:
            fuzzed_targets = fuzzed_targets.union(targets)
            print('Seems like the campaign was successful, will add its target functions to the blacklist!')
        else:
            print('Seems like the campaign failed, will not add its target functions to the blacklist')
            folder = cmd[cmd.find('/opt/fuzzing'):]
            folder = folder[:folder.find(' ')]
            print(f'Deleting {folder}...')
            os.system(f'rm -rf {folder}')
            cmd_bl.append(cmd)
        time.sleep(5)
    else:
        print(f'Could not find any suitable fuzzing command for functions {executed_targets.difference(fuzzed_targets)}')
        break

