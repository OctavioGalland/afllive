#ifndef _HARNESSER_H_
#define _HARNESSER_H_

#include "llvm/IR/GlobalVariable.h"

using namespace llvm;

class Harnesser {
public:
    void harnessMain(Module &M, Function *f);
    void harnessFunction(Module &M, Function *f, const std::set<unsigned int> &skipParams, const char *targetName);
    void instrumentFunction(Module &M, Function *f);

private:
    void addAFLDeclarations(Module &M);

    std::set<Module*> _visitedModules;
    std::set<Function*> _visitedTargetFunctions;
    std::set<Function*> _visitedInstrumentFunctions;
};

#endif // _HARNESSER_H_
