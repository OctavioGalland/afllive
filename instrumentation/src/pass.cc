#include <iostream>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/PassManager.h"

#include "json.hpp"

#include "Harnesser.h"
#include "utils_common.h"

namespace llvm {

class HarnessPass : public PassInfoMixin<HarnessPass> {
public:
    PreservedAnalyses run(Module &F, ModuleAnalysisManager &AM);
private:
    Harnesser _harnesser;
};

} // namespace llvm

using namespace llvm;
using json = nlohmann::json;


std::string getTypeName (Type *t, json &types) {
    static std::map<Type*, std::string> typeNames;
    if (typeNames.count(t) == 0) {
        size_t nextIdx = 0;
        for (const auto &it : types.items()) {
            if (it.key().substr(0, 5) == "type_") {
                size_t idx = std::stoi(it.key().substr(5)) + 1;
                if (idx > nextIdx)
                    nextIdx = idx;
            }
        }
        typeNames[t] = "type_" + std::to_string(nextIdx);
    }
    return typeNames[t];
}

std::string registerType (Module &M, Type *t, json &types) {
    static std::set<Type*> visitedTypes;
    std::string typeName = getTypeName(t, types);
    if (types.count(typeName) == 0) {
        json typeData = json::object();
        types[typeName] = typeData;
        if (t->isIntegerTy()) {
            typeData["basicType"] = "integer";
            typeData["bitWidth"] = t->getIntegerBitWidth();
        } else if (t->isFloatingPointTy()) {
            typeData["basicType"] = "floating";
            typeData["bitWidth"] = t->getScalarSizeInBits();
        } else if (t->isPointerTy()) {
            typeData["basicType"] = "pointer";
            if (t->isOpaquePointerTy()) {
                errs() << "Cannot populate opaque pointers!\n";
                abort();
            }
            Type *pointee = t->getNonOpaquePointerElementType();
            typeData["pointeeType"] = registerType(M, pointee, types);
        } else if (t->isStructTy()) {
            StructType *st = (StructType*)t;
            typeData["basicType"] = "struct";
            json fields = json::array();
            const DataLayout &dl = M.getDataLayout();
            const StructLayout *sl = dl.getStructLayout(st);
            for (size_t i = 0; i < st->getNumElements(); i++) {
                json field = {};
                uint64_t offset = sl->getElementOffset(i);
                Type *fieldType = st->getElementType(i);
                field["offset"] = offset;
                field["type"] = registerType(M, fieldType, types);
                fields[i] = field;
            }
            typeData["fields"] = fields;
            typeData["size"] = sl->getSizeInBytes();
        } else if (t->isArrayTy()) {
            typeData["basicType"] = "array";
            typeData["elementType"] = registerType(M, t->getArrayElementType(), types);
            typeData["length"] = t->getArrayNumElements();
        } else {
            // errs() << "Unrecognized type: " << *t << "!\n";
            // abort();
            typeData["basicType"] = "unsupported";
        }
        types[typeName] = typeData;
    }
    return typeName;
}

std::vector<std::string> targets;


PreservedAnalyses HarnessPass::run (Module &M,
                                      ModuleAnalysisManager &MAM) {
    const char *configFilePath = std::getenv("AH_CONFIG");
    if (configFilePath) {
        json config = json::parse(std::ifstream(configFilePath));
        Function *fMain = M.getFunction("main");
        if (fMain != NULL) {
            _harnesser.harnessMain(M, fMain);
        }
        if (std::getenv("AH_INSTRUMENT_ALL")) { // we keep this as an env variable because this is supposed to change throughout the build
           for (auto &f : M.functions()) {
               _harnesser.instrumentFunction(M, &f);
           }
        }
        json skip = {};
        if (config.count("skip") > 0) {
            skip = config["skip"];
        }
        if (config.count("targets") > 0) {
            std::vector<std::string> targets = config["targets"];
            for (std::string &targetFunctionName : targets) {
                size_t colonLoc = targetFunctionName.find(':');
                std::string moduleName = "";
                if (colonLoc != std::string::npos) {
                    moduleName = targetFunctionName.substr(0, colonLoc);
                    targetFunctionName = targetFunctionName.substr(colonLoc + 1);
                    if (!areModulesEquivalent(moduleName, M.getName().str())) {
                        continue;
                    }
                }
                Function *f = M.getFunction(targetFunctionName);
                std::set<uint32_t> skipParameters;
                for (auto &it : skip.items()) {
                    if (it.key() == targetFunctionName || areModuleColonFunctionNamesEquivalent(it.key(), M.getName().str() + ":" + targetFunctionName)) {
                        skipParameters = it.value().get<std::set<uint32_t>>();
                        break;
                    }
                }

                if (f != NULL && !f->isDeclaration()) {
                    _harnesser.harnessFunction(M, f, skipParameters, targetFunctionName.c_str());
                    // std::string modulePrefixedFunName = (M.getName() + ":" + f->getName()).str();
                    std::string modulePrefixedFunName = f->getName().str();
                    // We use a dedicated lock file to avoid race conditions because the typeinfo file will be opened and closed several times
                    int lockFile = -1;
                    do {
                        lockFile = open("/tmp/autoharness.lck", O_EXCL | O_CREAT);
                        if (lockFile < 0)
                            usleep(50000);
                    } while (lockFile < 0);
                    close(lockFile);
                    json output = {};
                    std::string outputPath = config["typeinfo_file"];
                    const char *cOutputPath = outputPath.c_str();
                    if (FILE *dummy = fopen(cOutputPath, "r")) {
                        fclose(dummy);
                        output = json::parse(std::ifstream(cOutputPath));
                        if (output["targets"].count(modulePrefixedFunName) > 0) {
                            unlink("/tmp/autoharness.lck");
                            continue;
                        }
                            //output["targets"][targetFunctionName].erase(output["targets"][targetFunctionName].begin(), output["targets"][targetFunctionName].end());
                    }
                    json targetArgs = json::array();
                    for (size_t i = 0; i < f->arg_size(); i++) {
                        Argument *arg = f->getArg(i);
                        if (!arg->hasStructRetAttr() && skipParameters.count(i) == 0) {
                            Type *argType = arg->getType();
                            targetArgs.push_back(registerType(M, argType, output));
                        }
                    }
                    if (output.count("targets") == 0) {
                        output["targets"] = {{modulePrefixedFunName, targetArgs}};
                    } else {
                        output["targets"][modulePrefixedFunName] = targetArgs;
                    }
                    std::ofstream(cOutputPath) << std::setw(4) << output << std::endl;
                    unlink("/tmp/autoharness.lck");
                }
            }
        } else {
            errs() << "[AH] No target specified!\n";
        }
        return PreservedAnalyses::none();
    } else {
        errs() << "[AH] No target specified!\n";
        return PreservedAnalyses::all();
    }
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo () {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "HarnessPass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(HarnessPass());
                });
        }
    };
}
