#ifndef _TYPES_H_
#define _TYPES_H_

#include <cstdint>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <functional>

using namespace std;

class Type;
typedef function<void(void*, const Type*)> Constraint;

class Type {
public:
    bool constraintsImplyEquality(const std::vector<Constraint> &cts) const;
    virtual void populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const = 0;
    virtual void serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const = 0;
    virtual void recordConstraintValue(void *to, void*from) const;
    virtual unsigned getSize() const = 0;
};

class StructType : public Type {
public:
    StructType(unsigned size);
    void populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void addField(shared_ptr<Type> t, unsigned offset);
    unsigned getSize() const;
private:
    unsigned _size;
    vector<pair<shared_ptr<Type>, unsigned>> _fields;
};

class PointerType : public Type {
public:
    PointerType(shared_ptr<Type> pointee);
    void populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void recordConstraintValue(void *to, void *from) const;
    unsigned getSize() const;
private:
    shared_ptr<Type> _pointee;
};

class IntegerType : public Type {
public:
    IntegerType(unsigned bitWidth);
    void populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void recordConstraintValue(void *to, void *from) const;
    unsigned getSize() const;
private:
    unsigned _bitWidth;
};

class FloatingType : public Type {
public:
    FloatingType(unsigned bitWidth);
    void populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void recordConstraintValue(void *to, void *from) const;
    unsigned getSize() const;
private:
    unsigned _bitWidth;
};

class ArrayType : public Type {
public:
    ArrayType(shared_ptr<Type> elemType, unsigned numElems);
    void populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    void serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints = {}) const;
    unsigned getSize() const;
private:
    shared_ptr<Type> _elemType;
    unsigned _numElems;
};

#endif // _TYPES_H_
