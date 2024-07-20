# Getting coverage for fuzzing campaigns

The `getcov.py` script expects the following fields: `project, iteration, binary, run_cmd, config_filepath, corpus_folder`, which will be describe in turn down below:

- `project`: name of the project (will be included in the output file)
- `iteration`: indicates which of the 20 execution this one represents (will later be used by [average_covs.py](https://gitlab.mpi-sp.org/octavio.galland/ah-evaluation/-/blob/main/results/average_covs.py) to average together output from different iterations)
- `binary`: the binary which `llvm-cov` should extract coverage from. This should either be the statically compiled binary being fuzzed or the shared object of the target library.
- `run_cmd`: the command line that should be used to execute the target
- `config_filepath`: the path to the config file. It will be modified to target each function in the corpus (so be careful about aborting the coverage collection process since it could corrput the file!).
- `corpus_folder`: the folder where the queue entries reside. If you copy the `default/queue` folder from the output folder of `afl-fuzz` be sure to copy timestamps as well (`cp -p`) since they're used to know which queue entries to include in which window.

The resulting file will look like:
```
[project]_iter_[iteration]@0: [lines covered]/[total lines]
[project]_iter_[iteration]@1: [lines covered]/[total lines]
...
```

This format will later be feed to `average_covs.py` to average all the iterations at each time window into a single dataframe which can then be plotted.

# Getting coverage for multiple fuzzing campaigns

When setting up the test amplification approach multiple campaigns will be set up sequentially. In some projects (namely, OpenSSL) every campaign runs within a different directory on different binaries, so the original coverage collection script falls short in some important ways.
In order to collect coverage from a test amplification campaign, a copy (including metadata) of `/opt/fuzzing` is needed. Another requirement is a json file containing the working directory for each fuzzing campaign (which as of now needs to be generated manually by the user).
There is a lot of coupling between this script and the setup created by [this script](https://gitlab.mpi-sp.org/octavio.galland/function-picker/-/blob/master/setup_fuzzers.py), so we'll go over how this is supposed to be used here without going into too much detail lest this documentation get outdated really quickly.

The required parameters are `project, iteration, binary, config_filepath, fuzz_root, pwds`. The first 4 are analogous to the previous script, the fifth argument should by an exact copy of the `/opt/fuzzing` folder created by the aforementioned script which orchestrates the fuzzing campaigns.
As for the last one, is should specify from which folder one needs to run each test. More specifically, during test amplification, when each fuzzing campaign gets created it gets assigned an output folder named `/opt/fuzzing/fuzzer_[some number]`. Each one of these campaigns needs runs in a potentially different working directory, since the original test suite may sometimes depend on the user running it from a specific folder in order for it to work. The problem is that when reproducing the campaigns there is no trace left of where each campaign was started, so it's up to the user to know which fuzzing campaign (as identified by the name of their output folder `/opt/fuzzing/fuzzer_[id]`) needs to be run from which directory.
This will surely be made clear with an [example](/example/README.md).

# DISCLAIMERS

When an exit condition is reached in libgen, a call to `_exit` (notice the underscore) gets made in order to prevent some spurious crashes that result from calling `exit` in some contexts (this should be looked into, ASAN was reporting some memory safety violation _within_ calls to `exit`, super weird). But this does not play nicely with clang's coverage collection, so most likely you'll want to replace calls to `_exit` with calls to `exit` in libgen before running the coverage scripts (and probably add coverage flags to LDFLAGS in libgen's Makefile as well).

When getting coverage for a new target, some changes to the scripts are necessary. Namely, one has to add an entry to the `skipPred` dictionary, containing a lambda function which takes as input a line of `llvm-cov`'s output and its index (`llvm-cov`'s output lines look something like `folder/file.c [total regions] [missed regions] [region coverage] [total lines] [missed lines]...`), and decides whether that line should be ignore or not. This is needed in case the binary being used to collect coverage happens to include non-library code as well (for instance, if the binary comes from a host statically linked to the target library, we want our script to count only lines pertraining to the library, not the host).
