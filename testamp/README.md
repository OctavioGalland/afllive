## Running

Compile a project using `clang` with the option `-fpass-plugin=$REPO_ROOT/testamp/pass/pass.so` and link with `-L$REPO_ROOT/testamp/lib -lpick`.

### Example for OpenSSL

```
git clone https://github.com/openssl/openssl.git
cd openssl
CC=clang-14 CFLAGS="-fno-discard-value-names -g -O0 -fno-omit-frame-pointer -fno-inline -fpass-plugin=$REPO_ROOT/testamp/pass/pass.so" LDFLAGS="-L$REPO_ROOT/testamp/lib" ./config no-shared no-module -lpick
make -j$(nproc)
make test # don't parallelize this! it will probably result in a race condition when writing the log
cat /opt/fuzz.log
```

> `-fno-discard-value-names` is not strictly necessary, but will keep argument names when printing functions. It makes it easier for the user to assess whether the generated constraints were correct or not. Likewise, the rest of the options (apart from -fpass-plugin -lpick and -L) are not mandatory.

Note that this will print every 'interesting' function executed by every invocation to an instrumented binary, along with the parameters and environment variables passed to it. Currently it is up to the user to set up the fuzzing campaign using this information.

## What does it do?

The injected code will call the library when entering a function whose name matches any of the regex specified in `isNameInteresting` @ `pass/src/main.cc:83` which takes an `u8*` or a `u8*` followed by a `u32` (or `u64`) as argument. It will take a guess at what the constraints and skipped arguments should be for that function and print all that information in `/opt/fuzz.log`. Additionally, it will print the environment and command line used to execute the program, so that the user can later reproduce the environment under `afl-fuzz`.
