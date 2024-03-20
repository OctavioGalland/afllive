#include "utils.h"

#include <sstream>

#include "llvm/IR/DerivedTypes.h"

std::string getNameForType (Type *t) {
    std::string res = "";
    static long unsigned lastUsedIndex = 0;
    if (t->isIntegerTy()) {
        res += "int_" + std::to_string(t->getIntegerBitWidth());
    } else if (t->isFloatingPointTy()) {
        res += "float_" + std::to_string(t->getPrimitiveSizeInBits());
    } else if (t->isStructTy()) {
        StructType *st = (StructType*)t;
        if (st->hasName()) {
            res += "struct_";
            res = res + st->getName().str();
        } else {
            res += "struct_anon_" + std::to_string(lastUsedIndex++);
        }
    } else if (t->isPointerTy()) {
        if (t->isOpaquePointerTy()) {
            res += "ptr_opaque_" + std::to_string(lastUsedIndex++);
        } else {
            res += "ptr_" + getNameForType(t->getNonOpaquePointerElementType());
        }
    }
    return res;
}

std::string indentText (const std::string &text, size_t indentLevel) {
    std::istringstream iss(text);
    std::string res = "";
    std::string indentPrefix = "";
    for (size_t i = 0; i < indentLevel; i++)
        indentPrefix += "\t";
    for (std::string line; std::getline(iss, line); )
    {
        res += indentPrefix + line + "\n";
    }
    return res;
}

std::string dumpType (Type *t) {
    std::string res;
    if (t->isArrayTy()) {
        res += "array to:\n";
        res += indentText(dumpType(((ArrayType*)t)->getElementType()));
    } else if (t->isStructTy()) {
        res += "struct\n";
        StructType *st = (StructType*)t;
        for (auto it = st->element_begin(); it != st->element_end(); it++) {
            res += "with field\n" + indentText(dumpType(*it));
        }
    } else if (t->isPointerTy()) {
        res += "pointer to:\n";
        res += indentText(dumpType(((PointerType*)t)->getPointerElementType()));
    } else if (t->isIntegerTy()) {
        res += "integer of " + std::to_string(t->getScalarSizeInBits()) + " bits\n";
    } else if (t->isFloatingPointTy()) {
        res += "fp of " + std::to_string(t->getScalarSizeInBits()) + " bits\n";
    }
    return res;
}