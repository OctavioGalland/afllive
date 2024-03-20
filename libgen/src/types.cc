#include "types.h"

#include <iostream>
#include <cstring>
#include <set>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "buff_utils.h"

extern char *filein;
extern char *__ah_shadow_function_name;
extern char __ah_arg_rec_failed;

bool Type::constraintsImplyEquality(const vector<Constraint> &constraints) const {
    bool equalityImplied = true;

    // This is super hacky, but since a constraint is just a function we have no way of knowing if a constraint (or a set of constraints)
    // imply an equality relationship with some other value. To make matters even worse constraints may treat a given value as being signed,
    // unsigned, or with different bit-widths. So, in order to __guess__ if a given set of constraint univocally determine the value of
    // a parameter, we try the set of constraints on all edge cases for integer values. If there is at least one pair of values which yield
    // different results when passed through the set of constraints, then we assume the constraints were jsut inequality relationships.
    // Surely costly in terms of run-time, so we better not call this from [SomeType]::populateToBuffer (serializeToBuffer is OK though,
    // it only gets called once (for now)). We don't include 64 bit pathological values since array lengths are always 32 bits.
    std::set<uint64_t> pathologicalValues = std::set<uint64_t> {0,
        (UINT32_MAX / 2), (UINT32_MAX / 2) + 1, UINT32_MAX,
        (UINT16_MAX / 2), (UINT16_MAX / 2) + 1, UINT16_MAX,
        (UINT8_MAX / 2), (UINT8_MAX / 2) + 1, UINT8_MAX};

    uint64_t lastResult = 0;
    bool firstRun = true;
    for (uint64_t value : pathologicalValues) {
        for (auto &it: constraints) {
            it(&value, this);
        }
        if (firstRun) {
            firstRun = false;
            lastResult = value;
        } else if (lastResult != value){
            equalityImplied = false;
        }
    }

    return equalityImplied;
}


void Type::recordConstraintValue(void *to, void *from) const {}

StructType::StructType(unsigned size) : _size(size) {
}

void StructType::populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    for (const auto &it : _fields) {
        void *fieldAddr = (void*)((char*)where + it.second);
        it.first->populateFromBuffer(buffer, buffer_size, offset, fieldAddr);
    }
}

void StructType::serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    for (const auto &it : _fields) {
        void *fieldAddr = (void*)((char*)where + it.second);
        it.first->serializeToBuffer(buffer, offset, fieldAddr);
    }
}

void StructType::addField(shared_ptr<Type> t, unsigned offset) {
    _fields.push_back(make_pair(t, offset));
}

unsigned StructType::getSize() const {
    return _size;
}

PointerType::PointerType(shared_ptr<Type> pointee) : _pointee(pointee) {
}

void PointerType::populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    bool deref = (size_t)where & 1;
    bool is_file = ((size_t)where) >> 63;
    where = (void*)((size_t)where & 0x7ffffffffffffffe);

    if (is_file) {
        // temp files are nice, but creating and deleting a file after every exec might not be optimal
        int fd = open(filein, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        if (buffer_size > *offset) {
            write(fd, buffer + *offset, buffer_size - *offset);
        }
        close(fd);
        *offset = buffer_size;
        *(char**)where = strdup(filein);
    } else {
        unsigned long numElems = (unsigned long)getBuffFour(buffer, buffer_size, offset);
        if (!deref) {
            for (const auto &it : constraints) {
                it(&numElems, this);
            }
        } else {
            numElems = 1;
        }
        // reserve space for a null element at the end too
        numElems++;
        unsigned pointeeSize = _pointee->getSize();
        void *res = malloc(pointeeSize * numElems);
        if ((!deref || constraints.empty()) && (dynamic_cast<IntegerType*>(_pointee.get()) || dynamic_cast<FloatingType*>(_pointee.get()))) {
            unsigned bytes = (numElems - 1) * pointeeSize;
            memcpy(res, buffer + *offset, bytes);
            *offset += bytes;
        } else {
            for (unsigned i = 0; i < numElems - 1; i++) {
                if (deref)
                    _pointee->populateFromBuffer(buffer, buffer_size, offset, (char*)res + i * pointeeSize, constraints);
                else
                    _pointee->populateFromBuffer(buffer, buffer_size, offset, (char*)res + i * pointeeSize);
            }
        }
        memset((char*)res + (numElems - 1) * pointeeSize, 0, pointeeSize);
        *(void**)where = res;
    }
}

static uint32_t getStringLength (void *where) {
    uint32_t numElems;
    for (numElems = 0; numElems < UINT32_MAX && (*(uint8_t**)where)[numElems] != 0; numElems++) {}
    if (numElems == UINT32_MAX) {
        std::cerr << "Something's gone terribly wrong, we've detected an array of size " << UINT32_MAX << "!\n";
        abort();
    }
    return numElems;
}

void PointerType::serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    bool deref = (uint64_t)where & 1;
    bool is_file = ((size_t)where) >> 63;
    where = (void*)((size_t)where & 0x7ffffffffffffffe);

    if (is_file) {
        char *name = *(char**)where;
        int fd = open(*(char**)where, O_RDONLY);
        if (fd == -1 ) {
            // std::cerr << "Could not open argument file '" << name << "' for target function '" << __ah_shadow_function_name << "'!\n";
            // abort();
            __ah_arg_rec_failed = 1;
            return;
        }
        struct stat st;
        fstat(fd, &st);
        size_t size = st.st_size;
        read(fd, buffer + *offset, size);
        close(fd);
        *offset += size;
    } else {
        unsigned long numElems = *(void**) where == NULL ? 0 : 1;
        // How do we know what is the length of a buffer if all we have is a pointer? This is really dirty, but in order to accomplish this we just
        // assume that the constraints will tell us the exact length, otherwise we assume length 1 (unless, of course, the pointer happens to be null).
        // This is just a heuristic that some programs seem to satisfy, pass a pointer to an element (in which case there is no constrain and numElems = 1),
        // or pass a buffer together with its length (in which case the user must better specify a constraint that tells us the size)

        // STUB: If we are recording arguments and we encounter a pointer to 8-bit integers without an equality constraint we assume it is a NULL-terminated
        // string and calculate it's length accordingly
        if (!constraintsImplyEquality(constraints) && dynamic_cast<IntegerType*>(_pointee.get()) && _pointee->getSize() == 1 && numElems != 0) {
            numElems = getStringLength(where);
        } else if (!deref) {
            for (const auto &it : constraints) {
                it(&numElems, this);
            }
            // If we got numElems = -1 (either as int or long) we may assume that the length is actually not determined
            if (numElems == 0xffffffff || numElems == 0xffffffffffffffff) {
                numElems = getStringLength(where);
            }
        }
        *(unsigned int *)(buffer + *offset) = numElems;
        *offset += 4;
        size_t pointeeSize = _pointee->getSize();
        for (size_t i = 0; i < numElems; i++) {
            if (deref)
                _pointee->serializeToBuffer(buffer, offset, *(char**)where + (i * pointeeSize), constraints);
            else
                _pointee->serializeToBuffer(buffer, offset, *(char**)where + (i * pointeeSize));
        }
    }
}

unsigned PointerType::getSize() const {
    return 8;
}

void PointerType::recordConstraintValue(void *to, void *from) const {
    uint32_t numElems = *(void**)from == NULL ? 0 : 1;
    // only attempt to infer length if we are pointing to a string-like object
    if (numElems != 0 && dynamic_cast<IntegerType*>(_pointee.get()) && _pointee->getSize() == 1) {
        numElems = getStringLength(from);
    }
    memcpy(to, &numElems, 4);
}

IntegerType::IntegerType(unsigned bitWidth) : _bitWidth(bitWidth) {
}

void IntegerType::populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    uint64_t res = 0;
    switch (_bitWidth) {
        case 8:
            res = getBuffOne(buffer, buffer_size, offset);
            break;
        case 16:
            res = getBuffTwo(buffer, buffer_size, offset);
            break;
        case 32:
            res = getBuffFour(buffer, buffer_size, offset);
            break;
        case 64:
            res = getBuffEight(buffer, buffer_size, offset);
            break;
        default:
            std::cerr << "Integer type with odd bit width: " << _bitWidth << "!\n";
            abort();
            break;
    }
    for (const auto &it : constraints) {
        it(&res, this);
    }
    memcpy(where, &res, getSize());
}

void IntegerType::serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    memcpy(buffer + *offset, where, getSize());
    *offset += getSize();
}

unsigned IntegerType::getSize() const {
    return _bitWidth / 8;
}

void IntegerType::recordConstraintValue(void *to, void *from) const {
    memcpy(to, from, getSize());
}

FloatingType::FloatingType(unsigned bitWidth) : _bitWidth(bitWidth) {
}

void FloatingType::populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    double res;
    switch (_bitWidth) {
        case 32:
            ((unsigned*)&res)[0] = getBuffFour(buffer, buffer_size, offset);
            break;
        case 64:
            ((unsigned long*)&res)[0] = getBuffEight(buffer, buffer_size, offset);
            break;
        default:
            std::cerr << "Floating type with odd bit width: " << _bitWidth << "!\n";
            abort();
            break;
    }
    for (const auto &it : constraints) {
        it(&res, this);
    }
    memcpy(where, &res, getSize());
}

void FloatingType::serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    memcpy(buffer + *offset, where, getSize());
    *offset += getSize();
}

unsigned FloatingType::getSize() const {
    return _bitWidth / 8;
}

void FloatingType::recordConstraintValue(void *to, void *from) const {
    memcpy(to, from, getSize());
}

ArrayType::ArrayType(shared_ptr<Type> elem, unsigned numElems) : _elemType(elem), _numElems(numElems) {
}

void ArrayType::populateFromBuffer(unsigned char *buffer, unsigned buffer_size, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    for (unsigned i = 0; i < _numElems; i++) {
        _elemType->populateFromBuffer(buffer, buffer_size, offset, (char*)where + i * _elemType->getSize());
    }
}

void ArrayType::serializeToBuffer(unsigned char *buffer, unsigned *offset, void *where, const vector<Constraint> &constraints) const {
    for (unsigned i = 0; i < _numElems; i++) {
        _elemType->serializeToBuffer(buffer, offset, (char*)where + i * _elemType->getSize());
    }
}

unsigned ArrayType::getSize() const {
    return _elemType->getSize() * _numElems;
}

