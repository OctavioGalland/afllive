#ifndef _CONSTRAINT_H_
#define _CONSTRAINT_H_

#include <functional>

#include "types.h"

enum Kind {
    EQ,
    LE,
    GE
};

class ConstraintFactory {
public:
    ConstraintFactory(Kind kind, bool isSigned);
    virtual Constraint getConstraint() const = 0;
protected:
    void constrain(const Type *lhsType, void *lhs, void *rhs, bool isSigned) const;
    Kind _kind;
    bool _isSigned;
};

class ConstantConstraintFactory : public ConstraintFactory{
public:
    ConstantConstraintFactory(Kind kind, uint64_t cnt, bool isSigned);
    Constraint getConstraint() const;
private:
    uint64_t _rhsConstant;
};

class ArgConstraintFactory : public ConstraintFactory{
public:
    ArgConstraintFactory(Kind kind, Type *t, void *ptr, bool isSigned);
    Constraint getConstraint() const;
private:
    Type *_rhsType;
    void *_rhsPtr;
};

#endif // _CONSTRAINT_H_
