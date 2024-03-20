#include "Harnesser.h"

#include <llvm-14/llvm/IR/InstrTypes.h>
#include <vector>

#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

static void defineNamedCharArray (Module &M, const std::string &name, const std::string &value) {
    LLVMContext &ctx = M.getContext();
    std::vector<Constant*> values;
    for (size_t i = 0; i < value.size() + 1; i++) {
        unsigned char c = value.c_str()[i];
        APInt val = APInt(8, (unsigned int)c);
        values.push_back(Constant::getIntegerValue(Type::getInt8Ty(ctx), val));
    }
    ArrayType *arrayType = ArrayType::get(Type::getInt8Ty(ctx), value.size() + 1);
    Constant *c = ConstantArray::get(arrayType, values);
    GlobalVariable *a = new GlobalVariable(M, arrayType, true, GlobalValue::LinkageTypes::PrivateLinkage, c, name);
    a->setAlignment(MaybeAlign(1));
    a->setUnnamedAddr(GlobalVariable::UnnamedAddr::Global);

    GlobalVariable *used = M.getGlobalVariable("llvm.compiler.used");
    auto initializerValues = std::vector<Constant*>{ConstantExpr::getBitCast(a, Type::getInt8PtrTy(ctx))};
    if (used == NULL) {
        auto *arrayType = ArrayType::get(Type::getInt8PtrTy(ctx), 1);
        Constant *initializer = ConstantArray::get(ArrayType::get(Type::getInt8PtrTy(ctx), 1), initializerValues);
        used = new GlobalVariable(M, arrayType, false, GlobalValue::LinkageTypes::AppendingLinkage, initializer, "llvm.compiler.used");
        used->setSection("llvm.metadata");
    } else {
        auto *oldInit = used->getInitializer();
        ConstantDataArray *oldArrInit = (ConstantDataArray*)oldInit;
        for (auto i = 0; i < oldArrInit->getNumElements(); i++) {
            initializerValues.push_back(oldArrInit->getAggregateElement(i));
        }
        auto *arrayType = ArrayType::get(Type::getInt8PtrTy(ctx), initializerValues.size());
        Constant *initializer = ConstantArray::get(arrayType, initializerValues);
        used->setName("");
        GlobalVariable *newUsed = new GlobalVariable(M, arrayType, false, GlobalValue::LinkageTypes::AppendingLinkage, initializer, "llvm.compiler.used");
        newUsed->setSection("llvm.metadata");
        for (auto it = used->user_begin(); it != used->user_end(); it++) {
            for (auto opIdx = 0; opIdx < it->getNumOperands(); opIdx++) {
                if (it->getOperand(opIdx) == used) {
                    it->setOperand(opIdx, newUsed);
                }
            }
        }
    }
}

static void declareNamedInt8Ptr (Module &M, const std::string &name) {
    LLVMContext &ctx = M.getContext();
    auto type = Type::getInt8PtrTy(ctx);
    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::InternalLinkage, Constant::getNullValue(type), name);
    a->setAlignment(MaybeAlign(8));
}

static void declareAFLFuzzLenInt32Ptr (Module &M) {
    LLVMContext &ctx = M.getContext();
    auto type = Type::getInt32PtrTy(ctx);
    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::ExternalLinkage, nullptr, "__afl_fuzz_len");
    a->setAlignment(MaybeAlign(8));
}

static void declareAHInFuzz (Module &M) {
    LLVMContext &ctx = M.getContext();
    auto type = Type::getInt8Ty(ctx);
    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::ExternalLinkage, nullptr, "__ah_in_fuzz");
    a->setAlignment(MaybeAlign(1));
}

static void defineAFLFuzzAltInt8Arr (Module &M) {
    LLVMContext &ctx = M.getContext();
    auto type = ArrayType::get(Type::getInt8Ty(ctx), 1048576);
    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::ExternalLinkage, ConstantAggregateZero::get(type), "__afl_fuzz_alt");
    a->setAlignment(MaybeAlign(16));
    a->setDSOLocal(true);
}

static void defineAFLFuzzAltPtrInt8Ptr (Module &M) {
    LLVMContext &ctx = M.getContext();
    auto type = ArrayType::getInt8PtrTy(ctx);
    auto pointeeType = ArrayType::get(Type::getInt8Ty(ctx), 1048576);
    std::vector<Value*> idxs;
    idxs.push_back(Constant::getIntegerValue(Type::getInt32Ty(ctx), APInt(32, 0)));
    idxs.push_back(Constant::getIntegerValue(Type::getInt32Ty(ctx), APInt(32, 0)));

    auto gepInitializer = ConstantExpr::getGetElementPtr(pointeeType, M.getNamedValue("__afl_fuzz_alt"), idxs);

    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::ExternalLinkage, gepInitializer, "__afl_fuzz_alt_ptr");
    a->setAlignment(MaybeAlign(8));
    a->setDSOLocal(true);
}

static void declareAFLFuzzPtrInt8Ptr (Module &M) {
    LLVMContext &ctx = M.getContext();
    auto type = Type::getInt8PtrTy(ctx);
    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::ExternalLinkage, nullptr, "__afl_fuzz_ptr");
    a->setAlignment(MaybeAlign(8));
}

static void defineAFLSharedmemFuzzingInt32 (Module &M) {
    LLVMContext &ctx = M.getContext();
    auto type = Type::getInt32Ty(ctx);
    GlobalVariable *a = new GlobalVariable(M, type, false, GlobalValue::LinkageTypes::ExternalLinkage, Constant::getIntegerValue(type, APInt(32, 1)), "__afl_sharedmem_fuzzing");
    a->setAlignment(MaybeAlign(4));
    a->setDSOLocal(true);
}

static void declare_AInt8Ptr (Module &M, const std::string &prefix) {
    const std::string name = prefix + "._A";
    declareNamedInt8Ptr(M, name);
}

static void declare_BInt8Ptr (Module &M, const std::string &prefix) {
    const std::string name = prefix + "._B";
    declareNamedInt8Ptr(M, name);
}

static void defineDeferSignature (Module &M) {
    defineNamedCharArray(M, "__DEFERED_SIGNATURE__", "##SIG_AFL_DEFER_FORKSRV##");
}


static void definePersistentSignature (Module &M) {
    defineNamedCharArray(M, "__PERSISTENT_SIGNATURE__", "##SIG_AFL_PERSISTENT##");
}

static void declareReadFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({
        Type::getInt32Ty(ctx),
        Type::getInt8PtrTy(ctx),
        Type::getInt64Ty(ctx)});
    Type *ret = Type::getInt64Ty(ctx);
    M.getOrInsertFunction("read", FunctionType::get(ret, args, false));
    // TODO: missing some attributes, shouldn't be a big deal though
}

static void declarePutsFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({
        Type::getInt8PtrTy(ctx)});
    Type *ret = Type::getInt32Ty(ctx);
    M.getOrInsertFunction("puts", FunctionType::get(ret, args, false));
}

static void declareMallocFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({
        Type::getInt64PtrTy(ctx)});
    Type *ret = Type::getInt8Ty(ctx);
    M.getOrInsertFunction("malloc", FunctionType::get(ret, args, false));
}

static void declareFreeFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({
        Type::getInt8PtrTy(ctx)});
    Type *ret = Type::getVoidTy(ctx);
    M.getOrInsertFunction("free", FunctionType::get(ret, args, false));
}

static void declareGenValuesFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({Type::getInt8PtrTy(ctx)});
    Type *ret = Type::getVoidTy(ctx);
    M.getOrInsertFunction("gen_values", FunctionType::get(ret, args, true));
}

static void declare__ah_epilogueFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({Type::getInt8PtrTy(ctx)});
    Type *ret = Type::getVoidTy(ctx);
    M.getOrInsertFunction("__ah_epilogue", FunctionType::get(ret, args, false));
}

static void declare__ah_beforeLoop(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({});
    Type *ret = Type::getVoidTy(ctx);
    M.getOrInsertFunction("__ah_beforeLoop", FunctionType::get(ret, args, false));
}

static void declareGenInitFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args;
    Type *ret = Type::getVoidTy(ctx);
    M.getOrInsertFunction("gen_init", FunctionType::get(ret, args, false));
}

static void declareAFLManualInitFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args;
    Type *ret = Type::getVoidTy(ctx);
    M.getOrInsertFunction("__afl_manual_init", FunctionType::get(ret, args, false));
}

static void declareAFLPersistentLoopFun(Module &M) {
    LLVMContext &ctx = M.getContext();
    std::vector<Type*> args({Type::getInt32Ty(ctx)});
    Type *ret = Type::getInt32Ty(ctx);
    M.getOrInsertFunction("__afl_persistent_loop", FunctionType::get(ret, args, false));
}

void Harnesser::addAFLDeclarations (Module &M) {
    if (_visitedModules.count(&M) == 0) {
        _visitedModules.insert(&M);
        declareGenValuesFun(M);
        declare__ah_epilogueFun(M);
    }
}

void Harnesser::harnessFunction (Module &M, Function *f, const std::set<unsigned int> &skipParams, const char *targetName) {
    if (f->isDeclaration()) return;
    if (_visitedTargetFunctions.count(f) != 0) {
        return;
    }
    // std::string moduleFunctionName = (M.getName() + ":" + f->getName()).str();
    std::string moduleFunctionName = targetName;// f->getName().str();
    errs() << "[AH] Harnessing: '" << moduleFunctionName << "'\n";
    _visitedTargetFunctions.insert(f);

    addAFLDeclarations(M);
    BasicBlock &firstBb = f->getEntryBlock();
    Instruction &firstInst = *(firstBb.begin());
    LLVMContext &funCtx = f->getContext();

    // Create and values for every non-returned, non-skipped argument
    std::vector<Value*> argPtrs;
    std::vector<Value*> argStores;
    for (size_t i = 0, argIdx = 0; i < f->arg_size(); i++) {
        Argument *arg = f->getArg(i);
        if (!arg->hasStructRetAttr() && skipParams.count(i) == 0) {
            Type *argType = arg->getType();
            AllocaInst *argPtrAllocInst = new AllocaInst(argType, 0, "allocArg" + std::to_string(argIdx), &firstInst);
            argStores.push_back(new StoreInst(f->getArg(i), argPtrAllocInst, &firstInst));
            argPtrs.push_back(argPtrAllocInst);
            argIdx++;
        }
    }

    defineNamedCharArray(M, "target_" + moduleFunctionName, moduleFunctionName);
    Value *castedName = new BitCastInst(M.getNamedGlobal("target_" + moduleFunctionName), Type::getInt8PtrTy(funCtx), "__ah_castedTarget", &firstInst);
    std::vector<Value*> initArgs{
        //M.getNamedGlobal("target_" + f->getName().str())
        castedName
    };
    for (size_t i = 0; i < argPtrs.size(); i++) {
        initArgs.push_back(argPtrs[i]);
    }

    Function *init = M.getFunction("gen_values");
    CallInst::Create(init->getFunctionType(), init, initArgs, "", &firstInst);

    for (size_t i = 0, argIdx = 0; i < f->arg_size(); i++) {
        Argument *arg = f->getArg(i);
        if (!arg->hasStructRetAttr() && skipParams.count(i) == 0) {
            Type *argType = arg->getType();
            LoadInst *argLoadInst = new LoadInst(argType, argPtrs[argIdx], "arg" + std::to_string(i), &firstInst);
            for (auto argUser : arg->users()) {
                if (argUser == argStores[argIdx]) {
                    continue;
                }
                for (size_t i = 0; i < argUser->getNumOperands(); i++) {
                    if (argUser->getOperand(i) == arg) {
                        argUser->setOperand(i, argLoadInst);
                    }
                }
            }
            argIdx++;
        }
    }

    Function *t = M.getFunction("__ah_epilogue");
    for (auto &bb : *f) {
      for (auto &ins : bb) {
        if (dyn_cast<ReturnInst>(&ins)) {
          CallInst::Create(t->getFunctionType(), t, std::vector<Value*>{castedName}, "", &ins);
        }
      }
    }
}

void Harnesser::instrumentFunction(Module &M, Function *f) {
    if (f->isDeclaration()) return;
    if (_visitedInstrumentFunctions.count(f) != 0) {
        return;
    }
    _visitedInstrumentFunctions.insert(f);
    errs() << "[AH] Instrumentign function: '" << f->getName() << "'\n";
    if (!M.getFunction("__ah_library_function")) {
        LLVMContext &ctx = M.getContext();
        std::vector<Type*> args;
        Type *ret = Type::getVoidTy(ctx);
        M.getOrInsertFunction("__ah_library_function", FunctionType::get(ret, args, false));
    }
    Function *ahLibFun = M.getFunction("__ah_library_function");
    Instruction &first = *f->getEntryBlock().begin();
    CallInst::Create(ahLibFun->getFunctionType(), ahLibFun, "unused", &first);
}

void Harnesser::harnessMain (Module &M, Function *f) {
    if (f->isDeclaration()) return;
    if (_visitedInstrumentFunctions.count(f) != 0) {
        return;
    }
    _visitedInstrumentFunctions.insert(f);
    errs() << "[AH] Harnessing main function: '" << f->getName() << "'\n";

    defineDeferSignature(M);
    if (M.getGlobalVariable("__afl_sharedmem_fuzzing") == NULL)
      defineAFLSharedmemFuzzingInt32(M);
    declareAFLManualInitFun(M);
    declareGenInitFun(M);

    if (std::getenv("AH_PERSISTENT") != NULL) {
        declareAFLPersistentLoopFun(M);
        definePersistentSignature(M);
        declare__ah_beforeLoop(M);
        LLVMContext &funCtx = f->getContext();
        // Wrap all code in a giant `while (__AFL_LOOP(X)) { ... }` loop
        size_t iterations = 1000;
        BasicBlock *first = &f->getEntryBlock();
        BasicBlock *guardBb = first->splitBasicBlock(first->begin(), "guard", true);
        BasicBlock *jmpToGuardBb = guardBb->splitBasicBlock(guardBb->begin(), "guard", true);
        BasicBlock *retBb = BasicBlock::Create(funCtx, "ret", f);

        // populate return bb
        Instruction *uniqueValidReturn = ReturnInst::Create(funCtx, Constant::getNullValue(Type::getInt32Ty(funCtx)), retBb);

        // populate guard bb
        Instruction *lastInGuard = guardBb->end()->getPrevNode();
        Function *__afl_loop = M.getFunction("__afl_persistent_loop");
        Instruction *shouldFuzz = CallInst::Create(__afl_loop->getFunctionType(), __afl_loop, std::vector<Value*>{Constant::getIntegerValue(Type::getInt32Ty(funCtx), APInt(32, iterations))}, "_afl_loop", lastInGuard);
        Function *__ah_beforeLoop = M.getFunction("__ah_beforeLoop");
        CallInst::Create(__ah_beforeLoop->getFunctionType(), __ah_beforeLoop, "", shouldFuzz);
        Value *shouldFuzzEq0 = new ICmpInst(lastInGuard, ICmpInst::Predicate::ICMP_EQ, shouldFuzz, Constant::getNullValue(Type::getInt32Ty(funCtx)), "shouldFuzz");
        BranchInst::Create(retBb, first, shouldFuzzEq0, lastInGuard);
        lastInGuard->eraseFromParent();

        // replace all returns (except ours) with jumps to first block
        std::vector<Instruction*> toErase;
        for (auto &bb : *f) {
            for (auto &ins : bb) {
                Instruction *insPtr = &ins;
                if (dyn_cast<ReturnInst>(insPtr) && insPtr != uniqueValidReturn) {
                    toErase.push_back(&ins);
                }
            }
        }
        for (auto &it : toErase) {
            BranchInst::Create(guardBb, it);
            it->eraseFromParent();
        }
    }


    Instruction &first = *f->getEntryBlock().begin();
    Instruction *firstPtr = &first;
    while (dyn_cast<AllocaInst>(firstPtr)) {
        firstPtr = firstPtr->getNextNode();
    }
    Function *genInitFun = M.getFunction("gen_init");
    CallInst::Create(genInitFun->getFunctionType(), genInitFun, "", firstPtr);
    // Function *aflManualInitFun = M.getFunction("__afl_manual_init");
    // CallInst::Create(aflManualInitFun->getFunctionType(), aflManualInitFun, "", firstPtr);
}

