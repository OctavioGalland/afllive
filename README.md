# AFLLive

The key idea is to fuzz a library within the context set up by a real world _host_ program using said library, thereby sidestepping the need to generate a fuzz-driver.
In order to achieve this we run the target library and host program through an LLVM pass specifying the location of a config file containing the various functions and parameters to be fuzzed through the `AH_CONFIG` environment variable. This pass will dump a description of the arguments that the target function receives to a json file, as well as instrumenting the binaries to make them interact with both AFL++ (setting up deferred forking and shared-memory access) and our runtime library `libgen`. As part of this instrumentation process, parameters passed from the host to a target function will be replaced with a new value of the same type provided by the fuzzer. During a fuzzing campaign, these alternative values for the target function's arguments are initialized by a function called `gen_values`, which is made available by the `libgen` library. This library will receive random data from the fuzzer, and use the type information generated earlier to _deserialize_ this random data stream into valid function arguments (the user can specify an arbitrary set of _constraints_ that the parameters of each function must satisfy in order to drive down the false-positive rate, if deemed necessary). These newly generated values will replace the ones provided by the host program, and will later be fed back to the fuzzer to allow for coverage-guidance.

## ICSE'25 artifacts

The artifacts for our [ICSE'25 paper](https://mpi-softsec.github.io/papers/ICSE25-invivo.pdf) (and instructions on how to run them) can be found under the [artifacts](artifacts) folder.

## Structure

This repository is structured as follows:

- [aflplusplus](aflplusplus): Our fork of `AFL++`, this component is in charge of orchestrating the fuzzing campaign, mutating inputs and detecting crashes.
- [instrumentation](instrumentation): An additional instrumentation pass to be used by our version of `afl-cc`. It hooks amplifier points and replaces their arguments with return values from `libgen`.
- [libgen](libgen): A run-time library against which instrumented binaries are linked. It communicates with [our fork of `AFL++`](aflplusplus), and de-serializes the byte-stream it provides with valid amplifier point parameters. It also serializes the initial set of parameters for each amplifier point, and sets a timer to terminate execution if the user requests it.
- [testamp](testamp): Contains various utilities to facilitate amplification of a whole test suite, potentially consisting of multiple binaries.
  - [testamp/pass](testamp/pass): Compilation pass which hooks amplifier points in order to record which ones are by which test run during test suite execution,
  - [testamp/lib](testamp/lib): Writes into `/opt/fuzz.log` the name of all functions executed by each test in the test suite, along with the environment variables, working directory, and command line paramters.
  - [testamp/setup\_fuzzers.py](testamp/setup_fuzzers.py): This script takes the output of running the previous two tools and greedily tries to run the least amount of tests that will execute all amplifier points recorded, allocating energy proportionately to the amount of tests being run.
- [coverage\_utils](coverage_utils/): Set of scripts utilized to record coverage on exeprimental subjects (see [artifacts](artifacts)).
- [config\_generator](config_generator/): Set of utilities to attempt to derive set of amplifier points and constraints for a given source-tree.
  - [config\_generator/interesting\_functions.ql](config_generator/interesting_functions.ql): Code-QL script that implements several heuristics to detect potential amplifier points.
  - [config\_generator/derive\_constraints.py](config_generator/derive_constraints.py): Python script that takes the output of the preeceding script and attempts to automatically derive constraints and produce a valid `AFLLive` configuration file.

## Running

### Prerequisites

This was implemented and tested in Ubuntu 22.04 and 24.04. This should work on other Linux distributions provided that `bash`, `boost` and LLVM 14 are installed:

```Shell
sudo apt-get update
sudo apt-get install -y build-essential bash clang-14 libclang-14-dev llvm-14 llvm-14-dev llvm-14-tools llvm-14-linker-tools llvm-14-runtime libboost-all-dev
```

Since this repository includes a fork `AFL++`, its dependencies are also needed (excerpt from [`AFL++`'s INSTALL.md](https://github.com/AFLplusplus/AFLplusplus/blob/v4.02c/docs/INSTALL.md)):

```Shell
sudo apt-get update
sudo apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev
# try to install llvm 14 and install the distro default if that fails
sudo apt-get install -y lld-14 llvm-14 llvm-14-dev clang-14 || sudo apt-get install -y lld llvm llvm-dev clang
sudo apt-get install -y gcc-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-plugin-dev libstdc++-$(gcc --version|head -n1|sed 's/\..*//'|sed 's/.* //')-dev
```

In order to automatically derive constriants, follow CodeQL's guide on how to install it: [https://docs.github.com/en/code-security/codeql-cli/getting-started-with-the-codeql-cli/setting-up-the-codeql-cli](https://docs.github.com/en/code-security/codeql-cli/getting-started-with-the-codeql-cli/setting-up-the-codeql-cli).

### Building

At a minimum, we need to build `aflplusplus`, `libgen` and `instrumentation`.

To build `AFL++` (as per upstream's instructions) run:

```Shell
cd afllpusplus
export LLVM_CONFIG=llvm-config-14
make all
sudo make install
cd ..
```

For `libgen` and `instrumentation` simply do:

```Shell
for component in libgen instrumentation
do
    cd $component
    make
    cd ..
done
```

Note that there is no need to install these components, as our run-time configuration file will tell the compiler where to find them.


### Example usage

In this section we'll illustrate basic `AFLLive` usage with a sample vulnerable program, named `vulnerable.c`:

```C
#include <stdlib.h>
#include <string.h>

void parse(char *bytes, size_t size) {
    if (size >= 4) {
        if (bytes[0] == 'b') {
            if (bytes[1] == 'a') {
                if (bytes[2] == 'd') {
                    if (bytes[3] == '!') {
                        abort();
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    char *buffer = "hello, world";
    parse(buffer, strlen(buffer));
    return 0;
}
```

> Note that in this example the function being fuzzed resides inside the host program, but in practice it will be provided by a separate library. The instructions provided here apply regardless, as long as all the code is compiled with our instrumentation.

In order to fuzz this program with `AFLLive` we need to compile it with our custom instrumentation, but first we need to do some setup.

#### Common configuration options

The fuzzer, as well as our instrumentation, will refer to a user-provided configuration file (in JSON format) to retrieve critical information during runtime.
The location of this file is indicated by the `AH_CONFIG` environment variable.

For starters, create an empty configuration file and set the relevant environment variable:

```Shell
touch config.json
export AH_CONFIG="$(pwd)/config.json"
```

It is mandatory to provide the location of the repository in the configuration file (so that `afl-cc` will be able to find the instrumentation and runtime libraries).
Additionally, during compilation, type information regarding amplification points needs to be dumped to a file (also in JSON format) so that it can later be retrieved, we also need to provide the path to this file as part of the configuration.
To do so, add the following content to it:

```
{
    "root": "<path to this repository>",
    "typeinfo_file": "/tmp/typeinfo.json"
}
```

#### Amplifier points and constraints

As part of our example we want the fuzzer to fuzz the `parse` function, and we need to instruct the fuzzer that the buffer needs to have a length equal to the integer passed as a second argument.
We achieve the former by setting the `targets` key inside the configuration JSON.

```
{
    "root": "<path to this repository>",
    "typeinfo_file": "/tmp/typeinfo.json",
    "targets": ["parse"]
}
```

> The `typeinfo_file` could point somewhere else, as we'd probably want to persist it. In this example it is stored in `/tmp` since it's a known user-writable path.

For the latter, we can provide a dictionary mapping each target name to a list of constraints that need to be observed:

```
{
    "root": "<path to this repository>",
    "typeinfo_file": "/tmp/typeinfo.json",
    "targets": ["parse"],
    "constraints": {
        "parse": [
            {
                "lhs": 0,
                "rhs": 1,
                "rel": "eq"
            },
            {
                "lhs": 1,
                "rhs": {
                    "constant": 512
                },
                "rel": "le"
            }
        ]
    }
}
```

What this set of constraints is expressing are two constraints:

- First, the first argument must have a length equal to the value of the second argument
- Second, the second argument must have a value less or equal than the constant 512 (to prevent suprious OOM errors)

Since the first parameter's value depends on the second one, and the second only depends in a constant, the fuzzer will deserialize a value into the second parameter first.
Once the second parameter is populated, the first one will be deserialized and then coerced into satisfying the constraint.

#### Performance tuning

The configuration file generated so far would allow us to fuzz the program.
But it would be inefficient for larger targets.

The main reason for this is that after an amplifier point is reached, `AFLLive` will start a forkserver, and each fork will continue until a crash is trigerred, or until program termination.
But intuitively we can think that a crash that results from mutation a function's parameters will ocurr shortly after that function is called, or it won't ocurr at all.
Leveraging this insight we can set a timer so that execution will stop a given amount of milliseconds after `parse` is called:

```
{
    "root": "<path to repo clone here>",
    "typeinfo_file": "/tmp/typeinfo.json",
    "targets": ["parse"],
    "constraints": {
        "parse": [
            {
                "lhs": 0,
                "rhs": 1,
                "rel": "eq"
            },
            {
                "lhs": 1,
                "rhs": {
                    "constant": 512
                },
                "rel": "le"
            }
        ]
    },
    "max_millis": 5
}
```

#### Compilation

Once the configuration file is set and the `AH_CONFIG` environment variable is pointing to it, we can compile the program using `afl-clang-fast`:

```Shell
afl-clang-fast -O0 -fno-omit-frame-pointer vulnerable.c -o vulnerable
```

> Note that here we disable optimizations since the output of our sample function isn't used and the compiler might discard it. In larger subjects this shouldn't be needed.

The output should include:

```
[AH] Harnessing main function: 'main'
[AH] Harnessing: 'parse'
```

This means the instrumentation hooked all relevant initialization into the `main` function, and amplifier-point-specific logic into the `parse` function.

We can inspect type information for the `parse` function by dumping the `typeinfo_file`:

```Shell
$ cat /tmp/typeinfo.json
{
    "targets": {
        "parse": [
            "type_0",
            "type_2"
        ]
    },
    "type_0": {
        "basicType": "pointer",
        "pointeeType": "type_1"
    },
    "type_1": {
        "basicType": "integer",
        "bitWidth": 8
    },
    "type_2": {
        "basicType": "integer",
        "bitWidth": 32
    }
}
```

#### Fuzzing

To start fuzzing, we just execute `afl-fuzz` as usual (omitting the input directory, since the initial corpus will be recorded at rutime):

```Shell
afl-fuzz -o out -- ./vulnerable
```

We'll get a modified `AFL++` status screen showing which amplifier point is currently being targeted, and after a while we'll get a crash.
At that point we can abort execution hitting Control-C.

#### Reproducing findings

The found crash will be located under `out/default/crashes/`, the exact name depending on the campaign.
We can examine the crash manually:

```
$ hexdump -C out/default/crashes/parse-*
00000000  08 56 dd 57 92 57 57 57  62 61 64 21              |.V.W.WWWbad!|
0000000c
```

We can see that the first 4 bytes are the value of the second parameter (the `lenght` parameter), since it exceeds 512 it will be capped during runtime.
The next 4 bytes are the length of buffer, which will be overridden during runtime with the value of `length`, and the remaining bytes are the contents of the buffer.

In order to reproduce the crash, we need to modify the configuration file to instruct the fuzzer to amplify the execution of `parse`, regardless of whether it's running as part of a campaign or not.
We do this by setting the `fuzz_target` key in the configuration file:

```
{
    "root": "<path to repo clone here>",
    "typeinfo_file": "/tmp/typeinfo.json",
    "targets": ["parse"],
    "constraints": {
        "parse": [
            {
                "lhs": 0,
                "rhs": 1,
                "rel": "eq"
            },
            {
                "lhs": 1,
                "rhs": {
                    "constant": 512
                },
                "rel": "le"
            }
        ]
    },
    "max_millis": 5,
    "fuzz_target": "parse"
}
```

When we run the binary again, it will read the input to deserialize from `stdin`, so we can just pipe the file output by `afl-fuzz`:

```Shell
$ ./vulnerable < out/default/crashes/parse-*
Aborted
```

## Configfile

The configuration should contain a json dictionary which _must_ include the following fields:
`typeinfo_file` (string): location of the file where relevant type information will be recorded.
`targets` (array of strings): name of target functions to execute.
`root` (string): path to the root of this repo within the local filesystem. This is needed by the compiler to invoke the required pass and link against `libgen`.

Additionally there are some other options whose usage will be examplified below.

## Buidling

Within the root of the repo, run:

```
# the following requires clang-14, llvm-config-14
cd libgen && make -j$(nproc)
cd instrumentation && make -j$(nproc)

# This overrides your local version of AFL++ with our fork
cd aflplusplus && LLVM_CONFIG=llvm-config-14 make -j$(nproc) all && sudo make install
```

## Running

One should set up the `AH_CONFIG` variable to point to a valid configuration file, and then compile the target project using [our fork of AFL++](aflplusplus).

Our modified version of `afl-clang-fast` will take care of inserting the pass in the compilation pipeline and link against the runtime library (that's why the config file needs to know the location of this repo, so that `afl-clang-fast` is able to find the necessary shared objects).

The resulting binary can be fuzzed with our version of `afl-fuzz`. This version works exactly the same as the original, but no longer needs the `-i` option to run. Instead, the input corpus will be recorded using the arguments supplied to initial calls to target functions upon initialization. It is important to note that at runtime it is still required to have the `AH_CONFIG` environment variable properly set, since it contains the constraints (more on this later), and the location of the file containing the relevant type information.

### Blacklisting functions at runtime

In some cases it is necessary to prevent the fuzzer from fuzzing certain functions, even if they are tagged as targets and are executed by the fuzzer (see `testamp/setup_fuzzers.py` for an example of when this is useful). In order to do this we allow the user to specify a list of functions to ignore in the `AH_BLACKLIST` environment variable. This variable must contain the name of every function to ignore surrounded by `:` characters, which delimit function names (so if one wants to blacklist functions `f` and `g` would do `AH_BLACKLIST=":f:g:" afl-fuzz ...`).
The reason we keep this setting as an environment variable instead of adding it to the config file is that this setting most likely needs to be changed on-the-fly when running multiple campaigns sequentially (or at least that was the use-case that motivated its implementation), so adding it to the config file would mean that a user would need to modify the json file in between campaigns, which is troublesome.

## Config file

### Autogeneration of config file (parser functions)

See [config\_generator](config_generator) for a script that will automatically identify parser-like functions and generate an appropriate config (requires [CodeQL](https://codeql.github.com/) in order to work). Note, however, that this config will be include absolute paths assuming that this repository is located under `/opt/afllive` (which was used when running our experiments within `Docker` but most likely **requires modification** for your use-case).

### Constraints for arguments

The `constraints` field of the `AH_CONFIG` file should contain a dictionary which looks like this:

```
{
    "target1": [constrain1, constrain2, ...],
    "target2": [constrain1', constrain2', ...],
    ...
}
```

Each constraint specifies conditions that a given argument need to satisfy, and it must have one of two forms (with one slight variation):

#### Binary constraints

```
{"lhs": argIdx, "rhs": argIdx, "rel": "[eq|ge|le]", "signed": [true|false]}
```

If the `signed` attribute is not specified, it will be treated as `false`. `argIdx` can be any argument index (starting from 0), and the `rel` attribute specifies the relation between those two arguments (must be equal, must be greater or equal, must be less or equal, respectively). The `signed` field specifies if the comparison done to determine whether the generated value (`lhs`) respects the constrain (`rhs`) should be done as either a signed or unsigned comparison.

In addition, the following is also accepted:

```
{"lhs": argIdx, "rhs": {"constant": numericConstant}, "rel": "[eq|le|ge]"}
```

which works exactly the same way, but uses a constant right hand side.

> It is important to note that these constraints imply possible orders in which the parameters can be popualted (if parameter `1` should be less than parameter `2`, then parameter `2` should be populated before populating parameter `1`. This is done in order to have the required information available when enforcing constraints). This means that there cannot be circular dependencies between values, since there would be no way to populate them! The user is therefore responsible for making sure no such circular dependencies arise.

> Another thing to note is that the numbers used to identify the arguments refers to their order ignoreing skipped arguments. For instance, if a function takes 5 arguments but the first and last one are marked as ignored (see `Skip arguments` down below), then the indices `0`, `1` and `2` within the constraints for that function refer to the second, third and fourth argument, respectively.

#### Unary constraints

Eventually the need arised for fuzzing files, for that reason we introduced unary constraints, which is just a way to tag some argument indices with certain properties. Right now the only unary constraint we support is `is_file`, which tags a char pointer as the name of a file (and instructs the fuzzer to create a file, fill it with fuzzer data and pass the name of the file as argument). Such a constraint looks like this:

```
{"lhs": argIdx, "is_file": true}
```

Keep in mind this only works for char pointers, and that wherever this constraint is specified it must be the only present constraint for that specific function.

#### Pointer types and the `deref` attribute

There is an optional attribute which can be added to any relationship, which is named `deref`. It should only be used on constraints applied to pointer types, and it makes the constraint in question work on the dereferenced value of the pointer.

For instance, if we have a target function with this signature: `void decode(int *measurements, unsigned int amountOfMeasurements)`, and we wanted each individual measurement to have a numeric value below, say, 255, we would specify this constraints:

```
{
    "decode": [
        {
            "lhs": 0,
            "rhs": 1,
            "rel": "eq"
        },
        {
            "lhs": 0,
            "rhs": {
                "constant: 255"
            },
            "rel":  "eq",
            "deref": true
        }
    ]
}
```

The first constraint makes the length of the `measurements` buffer be equal to the numeric value of `amountOfMeasurements`, and the second constraint will make each `int` inside the buffer be less than 255.

Just now we've quietly introduced the concept of buffers and length. To solve the ambiguity posed by pointer types (do they refer to a single element or to a buffer?) we treat each pointer as a potential array. So when we include a pointer in a relationship, we are actually predicating on the "length" of the pointer (that is, the length of the array pointed to by the pointer).

To make this point clear, we'll briefly go over a few examples:

- Given `f(char *a)`, to make the pointer point to a single element use: `{"lhs": 0, "rhs": {"constant": 1}, "rel": "eq"}`
- Given `f(char *a, int b)`, to make the pointer point to a buffer of at least `b` elements: `{"lhs": 0, "rhs": 1, "rel": "ge"}`
- Given `f(char **filenames)`, to make the pointer point to an array of length one with a valid filename in it: `[{"lhs": 0, "rhs": {"constant": 1}, "rel": "eq"}, {"lhs": 0, "is_file": true, "deref": true}]`

### Skip arguments

Another important field of the `AH_CONFIG` file is `skip`. It should contain a dictionary which associates an array containing the indices of arguments to be skipped with function names.

This is what it looks like:

```
{
    'f': [argIdxToBeSkipped0, argIdxToBeSkipped1...],
    ...
}
```

Recall that if you `skip`, say, argument index 0 then that argument will be completely ignored. By completely ignore we mean that the argument index 0 within the `constraints` field will now refer to the second argument instead of the first one.

### Initialization and de-initialization

There are many options which control how and when to both start the forkserver and terminate the child process, they are:

- `fork_on_init`: If specified, instructs the runtime to spin up the forkserver upon execution of the `main` function, instead of forking just before invoking the target function (which is the default). May improve stability on some targets.
- `max_millis`: If specified, sets the number of milliseconds that the shadow execution will be allowed to run for before forcefully terminating it.
- `exit_on_ret`: If specified, instructs the runtime to terminate the shadow execution just before returning from the target function.

(items below this point have been implemented earlier on, have not been used in experiments nor extensively tested)

- `max_calls`: If specified, sets the number of times any target function can be called after the shadow execution is terminated. This means that once the target selected by the fuzzer or the user (see `fuzz_target` option below) has been called and the arguments have been replaced, any call to any instrumented function will increase a counter. Once this counter reaches `max_calls`, execution will terminate.
- `max_fuzz`: If specified, sets the maximum number of times that a target function can be fuzzed, the default is 1. This means that if the same target function gets called twice and `max_fuzz` is 2, both calls will accept data from the fuzzer and replace their respective arguments. Usage of this option is discouraged since it reinforces the concept of structure within the input file (since one input now represents not only a sequence of arguments, but a sequence of sequences of arguments), which is best avoided for now.
- `fork_on_call`: If the `AH_INSTRUMENT_ALL` environment variable is set during compilation, then every library call will be instrumented and will invoke a different function for non-targets. What this variable does (when the binary has been compiled with said option) is set the number of times any (target or non-target) library function can be called before starting up the fork server. The inteded usage is to have finer grained control over when the fork server should start.

### Target selection (and reproduction of findings)

When running the target under `afl-fuzz` the target to fuzz will be selected by `afl-fuzz` and communicated to the fork-server using pipes. However, it is sometimes useful to manually select the target to fuzz, specially when trying to reproduce findings. To this end we implement a field `fuzz_target` which allows the user to run the binary outside of `afl-fuzz` and manually select the target function to fuzz.
Given a bug that's present in AFL++ 4.02c, when running the binary outside of afl it's recommended to set the environment variable `AFL_DISABLE_LLVM_INSTRUMENTATION` to `1` to avoid spurious crashes.
Also, when the fuzzing target gets executed it will attempt to read the fuzzing data from `stdin`, since that data would otherwise get read from shared memory set up by `afl-fuzz`.

### Initial target recording

As a preamble to the fuzzing campaign, original arguments need to be recorded into a buffer. However, if the constraint specification given by the user is not informative enough the recording phase may sometimes crash.
Since debugging the target binary under `afl-fuzz` is quite troublesome, we also implemented a `force_recording` option that makes the binary serialize the arguments of all target functions during normal execution, just as it would in an actual fuzzing campaign.
Just like before, be sure to use `AFL_DISABLE_LLVM_INSTRUMENTATION=1` when running the binary outside of afl-fuzz.
Another thing that is probably a good idea if the initial recording is not working, is to compile libgen with ASAN for better debugging.
