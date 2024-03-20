#include <iostream>
#include <llvm-14/llvm/IR/GlobalValue.h>
#include <llvm-14/llvm/IR/GlobalVariable.h>
#include <llvm-14/llvm/IR/Instructions.h>
#include <llvm-14/llvm/IR/LLVMContext.h>
#include <sstream>
#include <fstream>
#include <string>
#include <regex>

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Demangle//Demangle.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Support/FileSystem.h"

#include "json.hpp"

using json = nlohmann::json;

namespace llvm {

class HarnessPass : public PassInfoMixin<HarnessPass> {
public:
    PreservedAnalyses run(Module &F, ModuleAnalysisManager &AM);
private:
};

} // namespace llvm

using namespace llvm;

static FunctionCallee getEnterLibraryFunctionFunction (Module &M) {
    return M.getOrInsertFunction("__ah_enter_function", Type::getVoidTy(M.getContext()), Type::getInt8PtrTy(M.getContext()));
}

static FunctionCallee getInitFunctionFunction (Module &M) {
    return M.getOrInsertFunction("__ah_init", Type::getVoidTy(M.getContext()));
}

static FunctionCallee getDeinitFunctionFunction (Module &M) {
    return M.getOrInsertFunction("__ah_deinit", Type::getVoidTy(M.getContext()));
}

static void addFunctionCallAtBeginning (Function *f, FunctionCallee fc, std::vector<Value*> args) {
    auto &firstInst = *f->getEntryBlock().begin()->getNextNode();
    CallInst::Create(fc, args, "", &firstInst);
}

static Value *getFunctionSignatureConstant(Module &M, Function *f, std::string extras = "") {
    std::string globalName = "__function_signature_" + f->getName().str();
    GlobalValue *res;

    if ((res = M.getNamedGlobal(globalName)) == NULL) {
        LLVMContext &ctx = M.getContext();
        std::string signature = "";
        raw_string_ostream ros(signature);
        f->getReturnType()->print(ros);
        signature += " " + M.getName().str() + ":" + demangle(f->getName().str()) + " (";
        for (auto &arg : f->args()) {
            arg.getType()->print(ros);
            if (arg.hasName()) {
                signature += " " + std::string(arg.getName().data());
            }
            signature += ", ";
        }
        signature.pop_back();
        signature.pop_back();
        signature += ")" + extras;
        std::vector<Constant*> chars;

        for (size_t i = 0; i < signature.size(); i++) {
            chars.push_back(Constant::getIntegerValue(Type::getInt8Ty(ctx), APInt(8, signature.c_str()[i])));
        }
        chars.push_back(Constant::getIntegerValue(Type::getInt8Ty(ctx), APInt(8, 0)));
        ArrayType *arrayType = ArrayType::get(Type::getInt8Ty(ctx), signature.size() + 1);
        Constant *array = ConstantArray::get(arrayType, chars);
        res = new GlobalVariable(M, arrayType, true, GlobalValue::LinkageTypes::PrivateLinkage, array, globalName);
    }

    return res;
}

static bool isNameInteresting (std::string name) {
    bool res = false;

    std::string ahConfigFilePath = std::getenv("AH_CONFIG");
    if (ahConfigFilePath.empty()) {
        llvm::errs() << "[FP] AH_CONFIG not specified";
    } else {
        json config = json::parse(std::ifstream(ahConfigFilePath));
        std::vector<std::string> targets = config["targets"];
        res =  std::find(targets.cbegin(), targets.cend(), name) != targets.cend();
    }

    return res;
}

PreservedAnalyses HarnessPass::run (Module &M,
                                      ModuleAnalysisManager &MAM) {
    bool mainmodule = false;
    if (M.getFunction("main")) {
        mainmodule = true;
    }

    for (auto &f : M.functions()) {
        if (f.isDeclaration()) {
            continue;
        }

        FunctionCallee fc = getDeinitFunctionFunction(M);
        if (f.getName().str() == "main") {
            addFunctionCallAtBeginning(&f, getInitFunctionFunction(M), std::vector<Value*>{});
            for (auto &bb : f) {
                for (auto &inst : bb) {
                    if (dyn_cast<ReturnInst>(&inst)) {
                        CallInst::Create(fc.getFunctionType(), fc.getCallee(), "", &inst);
                    }
                }
            }
        }

        for (auto &bb : f) {
            for (auto &inst : bb) {
                CallInst *callInst;
                if ((callInst = dyn_cast<CallInst>(&inst)) != NULL) {
                    Function *calledFunction = callInst->getCalledFunction();
                    if (calledFunction) {
                        StringRef calledFunctionName = calledFunction->getName();
                        if (calledFunctionName == "exit") {
                            CallInst::Create(fc.getFunctionType(), fc.getCallee(), "", &inst);
                        }
                    }
                }
            }
        }

        Value *functionSignatureArr = getFunctionSignatureConstant(M, &f);
        Value *functionSignaturePtr = new BitCastInst(functionSignatureArr, Type::getInt8PtrTy(M.getContext()), "functionSignaturePtr", &*f.begin()->begin());
        std::vector<Value*> funSigArgs = std::vector<Value*>{functionSignaturePtr};

        if (isNameInteresting(f.getName().str()) && !mainmodule) {
            addFunctionCallAtBeginning(&f, getEnterLibraryFunctionFunction(M), funSigArgs);
        }
    }

    return PreservedAnalyses::none();
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
