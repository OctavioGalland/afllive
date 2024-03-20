#include "ConstraintFactory.h"

#include <cstring>

ConstraintFactory::ConstraintFactory(Kind k, bool isSigned) : _kind(k), _isSigned(isSigned) {
}

ConstantConstraintFactory::ConstantConstraintFactory(Kind k, uint64_t cnt, bool isSigned) : ConstraintFactory(k, isSigned), _rhsConstant(cnt) {
}

void ConstraintFactory::constrain(const Type *lhsType, void *lhs, void *rhs, bool isSigned) const {
    bool shouldCopy = false;
    if (dynamic_cast<const PointerType*>(lhsType)) {
        shouldCopy = false;
        unsigned int length = *(unsigned int*)lhs;
        unsigned int bound = *(unsigned int*)rhs;
        if (_kind == LE ? length > bound : length < bound) {
            memcpy(lhs, rhs, 4);
        }
    } else if (dynamic_cast<const FloatingType*>(lhsType)) {
        if (lhsType->getSize() == 4) {
            float rhsVal = *(float*)rhs;
            float lhsVal = *(float*)lhs;
            shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
        } else if (lhsType->getSize() == 8) {
            double rhsVal = *(double*)rhs;
            double lhsVal = *(double*)lhs;
            shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
        }
    } else if (dynamic_cast<const IntegerType*>(lhsType)) {
        if (lhsType->getSize() == 1) {
            if (isSigned) {
                char rhsVal = *(char*)rhs;
                char lhsVal = *(char*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            } else {
                unsigned char rhsVal = *(unsigned char*)rhs;
                unsigned char lhsVal = *(unsigned char*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            }
        } else if (lhsType->getSize() == 2) {
            if (isSigned) {
                short rhsVal = *(short*)rhs;
                short lhsVal = *(short*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            } else {
                unsigned short rhsVal = *(unsigned short*)rhs;
                unsigned short lhsVal = *(unsigned short*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            }
        } else if (lhsType->getSize() == 4) {
            if (isSigned) {
                int rhsVal = *(int*)rhs;
                int lhsVal = *(int*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            } else {
                unsigned int rhsVal = *(unsigned int*)rhs;
                unsigned int lhsVal = *(unsigned int*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            }
        } else if (lhsType->getSize() == 8) {
            if (isSigned) {
                long rhsVal = *(long*)rhs;
                long lhsVal = *(long*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            } else {
                unsigned long rhsVal = *(unsigned long*)rhs;
                unsigned long lhsVal = *(unsigned long*)lhs;
                shouldCopy = (_kind == LE ? lhsVal > rhsVal : lhsVal < rhsVal);
            }
        }
    }
    if (shouldCopy)
        memcpy(lhs, rhs, lhsType->getSize());
}

Constraint ConstantConstraintFactory::getConstraint() const {
    Constraint res;
    switch (_kind) {
        case EQ:
            res = [&](void *lhs, const Type *lhsType) {
                if (dynamic_cast<const PointerType*>(lhsType)) {
                    memcpy(lhs, &_rhsConstant, 4);
                } else {
                    memcpy(lhs, &_rhsConstant, lhsType->getSize());
                }
            };
            break;
        case LE:
        case GE:
            res = [&](void *lhs, const Type *lhsType) {
                constrain(lhsType, lhs, (void*)&_rhsConstant, _isSigned);
            };
            break;
        default:
            break;
    }
    return res;
}

ArgConstraintFactory::ArgConstraintFactory(Kind k, Type *t, void *ptr, bool isSigned) : ConstraintFactory(k, isSigned), _rhsType(t), _rhsPtr(ptr) {
}

Constraint ArgConstraintFactory::getConstraint() const {
    Constraint res;
    switch (_kind) {
        case EQ:
            res = [&](void *lhs, const Type *lhsType) {
                if (dynamic_cast<const PointerType*>(lhsType)) {
                    memcpy(lhs, _rhsPtr, 4);
                } else {
                    memcpy(lhs, _rhsPtr, lhsType->getSize());
                }
            };
            break;
        case LE:
        case GE:
            res = [&](void *lhs, const Type *lhsType) {
                constrain(lhsType, lhs, _rhsPtr, _isSigned);
            };
            break;
        default:
            break;
    }
    return res;
}

