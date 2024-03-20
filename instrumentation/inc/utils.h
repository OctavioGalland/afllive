#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>

#include "llvm/IR/Type.h"

using namespace llvm;

std::string getNameForType(Type *t);
std::string dumpType(Type *t);
std::string indentText(const std::string &text, size_t indentLevel = 1);

#endif // _UTILS_H_