#include <fstream>
#include <memory>
#include <set>
#include <iostream>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include "json.hpp"
#include "types.h"
#include "Dependencies.h"
#include "ConstraintFactory.h"
#include "utils_common.h"

#define __AFL_INIT() \
    do { \
        static volatile char *_A __attribute__((used,unused)); \
        _A = (char*)"DEFER_SIG"; \
        __attribute__((visibility("default"))) void _I(void) __asm__("__afl_manual_init"); \
        _I(); \
    } while (0)


using json = nlohmann::json;

unsigned char *buf = NULL;
unsigned int buf_len = 0;
unsigned int buf_offset = 0;

unsigned char *__ah_arg_fuzz_alt;
unsigned char *__ah_arg_fuzz_ptr;
extern unsigned char *__ah_arg_fuzz;
extern unsigned char __ah_recording;
unsigned int ahArgOffset = 0;

char __ah_in_fuzz = 0;
char __ah_arg_fuzz_initialized = 0;
char __ah_arg_rec_failed = 0;
char *__ah_shadow_function_name = 0;
uint32_t __ah_shadow_recurse = 0;
extern unsigned int *__afl_fuzz_len;
extern unsigned char *__afl_fuzz_ptr;

unsigned char __afl_fuzz_alt[1048576];
unsigned char *__afl_fuzz_alt_ptr = __afl_fuzz_alt;

#define __AFL_FUZZ_TESTCASE_BUF (__afl_fuzz_ptr ? __afl_fuzz_ptr : __afl_fuzz_alt_ptr)
#define __AFL_FUZZ_TESTCASE_LEN (__afl_fuzz_ptr ? *__afl_fuzz_len : (*__afl_fuzz_len = read(0, __afl_fuzz_alt_ptr, 1048576)) == 0xffffffff ? 0 : *__afl_fuzz_len)

extern char *__ah_current_target;

char *filein;

std::set<std::string> recordedFunctions;

// useful for testing and inserting constants at specified buffer positions
// #define set_at(buf, offset, type, val) (*(type *)((char*)buf + offset) = (type)(val))

std::map<std::string, std::vector<std::shared_ptr<Type>>> argTypes;
std::map<std::string, std::shared_ptr<Type>> knownTypes;

shared_ptr<Type> make_type(json config, const std::string &typeName) {
    if (knownTypes.count(typeName) > 0) {
        return knownTypes[typeName];
    } else {
        std::shared_ptr<Type> res;
        if (config[typeName]["basicType"] == "integer") {
            res = std::make_shared<IntegerType>(IntegerType(config[typeName]["bitWidth"]));
        } else if (config[typeName]["basicType"] == "floating") {
            res =  std::make_shared<FloatingType>(FloatingType(config[typeName]["bitWidth"]));
        } else if (config[typeName]["basicType"] == "pointer") {
            std::string pointeeTypeName = config[typeName]["pointeeType"];
            std::shared_ptr<Type> pointee = std::shared_ptr<Type>(make_type(config, pointeeTypeName));
            res = std::make_shared<PointerType>(PointerType(pointee));
        } else if (config[typeName]["basicType"] == "struct") {
            unsigned structSize = config[typeName]["size"];
            std::shared_ptr<StructType> st = std::make_shared<StructType>(StructType(structSize));
            knownTypes[typeName] = st;
            for (auto field : config[typeName]["fields"]) {
                unsigned fieldOffset = field["offset"];
                std::string fieldType = field["type"];
                st->addField(make_type(config, fieldType), fieldOffset);
            }
            res = st;
        } else if (config[typeName]["basicType"] == "array") {
            std::string elementTypeName = config[typeName]["elementType"];
            size_t length = config[typeName]["length"];
            res = std::make_shared<ArrayType>(ArrayType(make_type(config, elementTypeName), length));
        } else if (config[typeName]["basicType"] == "unsupported") {
            res = nullptr;
        } else {
            std::cerr << "Invalid type name: " << typeName << "!\n";
            abort();
        }
        knownTypes[typeName] = res;
        return res;
    }
}

inline bool file_exists (const char *name) {
    struct stat buffer;
    return (stat (name, &buffer) == 0);
}

std::vector<std::vector<Constraint>> constraints;

json userConstraints;

std::string userTarget;
std::string blacklist;

unsigned currentCalls = 0;
unsigned maxCalls = 5;
unsigned maxMillis = 10;
bool exitOnReturn = false;
bool exitOnTimer = false;
bool timerSet = false;
bool exitOnMaxCalls = false;
bool forkOnInit = false;
bool forceExit = false;

size_t maxFuzz = 1;
size_t fuzzIdx = 0;
size_t libraryCall = 0;
size_t forkOnCallNr = 0;

bool userRecording = false;

void alarm_handler(int unused) {
    if (userTarget.empty() || forceExit) {
        _exit(0);
    } else {
        exit(0);
    }
}

void* pthread_handler(void* unused) {
    usleep(maxMillis * 1000);
    if (userTarget.empty() || forceExit) {
        _exit(0);
    } else {
        exit(0);
    }
}

struct itimerval val = {.it_interval = {.tv_sec = 0, .tv_usec = 0}, .it_value = {.tv_sec = 0, .tv_usec = 1000 * maxMillis}};

extern "C" {
    void gen_init () {
        filein = (char*)malloc(256);
        snprintf(filein, 256, "/tmp/__ah_in_%ld.gsm", (long)getpid());
        const char *configPath = std::getenv("AH_CONFIG");
        if (configPath == NULL) {
            std::cerr << "AH_CONFIG not specified! This should be the path of the configuraton file!\n";
        } else if (!file_exists(configPath)) {
            std::cerr << "AH_CONFIG does not point to an actual, readable file!\n";
        } else {
            json config = json::parse(std::ifstream(configPath));

            // type info file
            if (config.count("typeinfo_file") > 0) {
                std::string typeinfoFilepath = config["typeinfo_file"];
                if (FILE* f = fopen(typeinfoFilepath.c_str(), "r")) {
                    fclose(f);
                    json typeinfo = json::parse(std::ifstream(config["typeinfo_file"]));
                    for (auto target : typeinfo["targets"].items()) { // targets are module-prefixed
                        for (auto argType : target.value().items()) {
                            argTypes[target.key()].push_back(make_type(typeinfo, argType.value()));
                        }
                    }
                } else {
                    std::cerr << "AH_CONFIG specifies an invalid typeinfo file path!\n";
                }
            } else {
                std::cerr << "AH_CONFIG file does not specify the location of the type info file!\n";
            }

            // constraints
            if (config.count("constraints") > 0) {
                userConstraints = config["constraints"];
            }

            // entrance and exit conditions
            // 1. fuzz target override
            if (config.count("fuzz_target") > 0) {
                userTarget = config["fuzz_target"];
            }
            // 2. max millis seconds after target function gets fuzzed
            if (config.count("max_millis") > 0) {
                exitOnTimer = true;
                maxMillis = config["max_millis"];
                val.it_value.tv_usec = 1000 * maxMillis;
                //signal(SIGVTALRM, alarm_handler);
            }
            // 3. max calls after target function gets fuzzed
            if (config.count("max_calls") > 0) {
                exitOnMaxCalls = true;
                maxCalls = config["max_calls"];
            }
            // 4. whether or not to exit upon target function return
            if (config.count("exit_on_ret") > 0) {
                exitOnReturn = config["exit_on_ret"];
            }
            // 5. recording override
            if (config.count("force_recording") > 0) {
                userRecording = config["force_recording"];
            }
            // 6. amount of occurences of target function to fuzz
            if (config.count("max_fuzz") > 0) {
                maxFuzz = config["max_fuzz"];
            }
            // 7. fork on program init
            if (config.count("fork_on_init") > 0) {
                forkOnInit = config["fork_on_init"];
            }
            // 8. fork on number of library calls
            if (config.count("fork_on_call") > 0) {
                forkOnCallNr = config["fork_on_call"];
            }
            // 9. Allow user to `_exit` after timeout
            if (config.count("force_exit") > 0) {
                forceExit = config["forceExit"];
            }
        }

        if (getenv("AH_BLACKLIST")) {
            blacklist = getenv("AH_BLACKLIST");
        }

        if (__ah_recording || forkOnInit) {
            __AFL_INIT();
        }
    }

    void __ah_beforeLoop () {
        __ah_in_fuzz = 0;
        currentCalls = 0;
        fuzzIdx = 0;
    }

    void gen_values (const char *fnName, ...) { // fnName is module-prefixed
        if (__ah_in_fuzz) {
            if (exitOnMaxCalls) {
                currentCalls++;
                if (currentCalls > maxCalls) {
                    _exit(0);
                }
            }
            if (!strcmp(fnName, __ah_shadow_function_name)) {
                __ah_shadow_recurse++;
            }
        }

        bool shouldRecord = (userRecording || __ah_recording) && (recordedFunctions.count(fnName) == 0) && (blacklist.find(fnName) == std::string::npos);
        bool shouldFuzz = (!userTarget.empty() && strcmp(userTarget.c_str(), fnName) == 0) || (__ah_current_target && !strcmp(fnName, __ah_current_target));
        shouldFuzz = shouldFuzz && !shouldRecord && (__ah_in_fuzz == 0 || fuzzIdx < maxFuzz);

        if (!forkOnInit && (forkOnCallNr == 0) && (shouldFuzz || shouldRecord || (__ah_current_target && !strcmp(__ah_current_target, "__AH_NON_EXISTENT__")))) {
            __AFL_INIT();
        }

        if (shouldFuzz) {
            __ah_in_fuzz = 1;
            __ah_shadow_function_name = strdup(fnName);
            __ah_shadow_recurse = 1;
            fuzzIdx++;
        }

        if (shouldRecord) {
            if (__ah_shadow_function_name != 0) {
                free(__ah_shadow_function_name);
            }
            __ah_shadow_function_name = strdup(fnName);
            if (!__ah_arg_fuzz_initialized) {
                __ah_arg_fuzz_ptr = __ah_arg_fuzz ? __ah_arg_fuzz : (__ah_arg_fuzz_alt = (unsigned char*)malloc((25 * 1024 * 1024L + 256 + 4) * 256));
                __ah_arg_fuzz_initialized = 1;
            }
            recordedFunctions.insert(fnName);
            memset(__ah_arg_fuzz_ptr + ahArgOffset, 0, 256);
            strncpy((char*)(__ah_arg_fuzz_ptr + ahArgOffset), fnName, 255);
            ahArgOffset += 256;
        }

        if (shouldFuzz || shouldRecord) {
            std::vector<void*> addressOfArg;
            std::vector<std::vector<Constraint>> constraintsOfArg;

            if (shouldFuzz) {
                buf = __AFL_FUZZ_TESTCASE_BUF;
                buf_len = __AFL_FUZZ_TESTCASE_LEN;
                buf_offset = 0;
            }

            va_list vl;
            va_start(vl, fnName);
            const auto &types = argTypes[fnName];
            size_t i = 0;
            for (const auto &it : types) {
                void *arg = va_arg(vl, void*);
                addressOfArg.push_back(arg);
                constraintsOfArg.push_back({});
                i++;
            }
            DependencyManager dm(i);

            // function name in config file may or may not specify a module, but the function given by the pass
            // always incldues the module name. Take that into account when searching for function constraints
            json userConstraintsForFunc = {}; //userConstraints[fnName];
            for (auto &it : userConstraints.items()) {
                if (areModuleColonFunctionNamesEquivalent(it.key(), fnName)) {
                    userConstraintsForFunc = it.value();
                    break;
                }
            }
            for (const auto &it : userConstraintsForFunc) {
                unsigned lhs = it["lhs"];
                if (it.count("is_file") > 0) {
                    uint64_t adr = (uint64_t)addressOfArg[lhs];
                    adr |= 0x8000000000000000;
                    addressOfArg[lhs] = (void*)adr;
                } else {
                    std::string relType = it["rel"];
                    bool signedComparison = false;
                    if (it.count("signed") > 0) {
                        signedComparison = it["signed"];
                    }
                    Kind k;
                    if (relType == "eq") {
                        k = EQ;
                    } else if (relType == "le") {
                        k = LE;
                    } else if (relType == "ge") {
                        k = GE;
                    }
                    ConstraintFactory *cf;
                    if (it["rhs"].is_number()) {
                        unsigned rhs = it["rhs"];
                        void *rhsPtr = addressOfArg[rhs];
                        std::shared_ptr<Type> rhsType = types[rhs];
                        dm.addDependency(rhs, lhs);
                        cf = new ArgConstraintFactory(k, rhsType.get(), rhsPtr, signedComparison);
                    } else if (it["rhs"].is_string() && it["rhs"] == "rec") {
                        uint64_t rhs = 0;
                        types[lhs]->recordConstraintValue(&rhs, addressOfArg[lhs]);
                        cf = new ConstantConstraintFactory(k, rhs, signedComparison);
                    } else {
                        uint64_t constant = it["rhs"]["constant"];
                        cf = new ConstantConstraintFactory(k, constant, signedComparison);
                    }
                    constraintsOfArg[lhs].push_back(cf->getConstraint());
                    if (it.count("deref") > 0) {
                        uint64_t adr = (uint64_t)addressOfArg[lhs];
                        adr |= 1;
                        addressOfArg[lhs] = (void*)adr;
                    }
                }
            }

            // Make one round to extract original args
            unsigned int previousAhArgOffset = ahArgOffset;
            ahArgOffset += 4;
            if (shouldRecord) {
                __ah_arg_rec_failed = 0;
                dm.visitInOrder([&types, &addressOfArg, &constraintsOfArg](unsigned arg) {
                    types[arg]->serializeToBuffer(__ah_arg_fuzz_ptr, &ahArgOffset, addressOfArg[arg], constraintsOfArg[arg]);
                });
                if (__ah_arg_rec_failed) {
                    ahArgOffset = previousAhArgOffset;
                    ahArgOffset -= 256;
                    memset(__ah_arg_fuzz_ptr + ahArgOffset, 0, 256);
                }
            }

            // Make second round to populate the new args
            if (shouldFuzz) {
                dm.visitInOrder([&types, &addressOfArg, &constraintsOfArg](unsigned arg) {
                    types[arg]->populateFromBuffer(buf, buf_len, &buf_offset, addressOfArg[arg], constraintsOfArg[arg]);
                });
                if (exitOnTimer && !timerSet) {
                    //setitimer(ITIMER_VIRTUAL, &val, NULL);
                    pthread_t newThread;
                    pthread_create(&newThread, NULL, pthread_handler, NULL);
                    timerSet = 1;
                }
            }
            // It is very important that the process of extracting the original arguments and creating the new ones is done in two rounds.
            // This happens because the relationships between elements will not be properly handled if they are modified while reading them
            if (shouldRecord && !__ah_arg_rec_failed) {
                *(unsigned int*)(__ah_arg_fuzz_ptr + previousAhArgOffset) = (ahArgOffset - (previousAhArgOffset + 4));
            }
        }
    }

    void __ah_library_function () {
        libraryCall++;
        if (forkOnCallNr != 0 && forkOnCallNr == libraryCall) {
            __AFL_INIT();
        }
    }

    void __ah_epilogue(char *fnName) {
        if (exitOnReturn && !__ah_recording && __ah_in_fuzz && !strcmp(fnName, __ah_shadow_function_name)) {
            __ah_shadow_recurse--;
            if (__ah_shadow_recurse == 0) {
                _exit(0);
            }
        }
    }
}

