#ifndef SING_VECTORS_H_
#define SING_VECTORS_H_

#include "memory.h"
#include <initializer_list>
#include <algorithm>

namespace sing {

// BASIC VECTOR (What receiving functions see !!)

template<class T>
class vect {
public:
    // destructor
    ~vect() {
        if (_allocated != 0 && _first != nullptr) {
            delete[] _first;
        }
    }

    // [] operator
    T& operator[](size_t i)
    {
        if (i >= _count) {
            if (i == _count) {
                if (_count >= _allocated) {
                    GrowTo(_count + 1);
                }
                _count++;
            } else {
                throw(std::out_of_range("array index"));
            }
        }
        return _first[i];
    }

    const T& operator[](size_t i) const
    {
        if (i >= _count) {
            throw(std::out_of_range("array index"));
        }
        return _first[i];
    }

    void operator=(const vect<T> &right) {
        if (right._count > _count) {
            GrowTo(right._count);
        }
        for (size_t ii = 0; ii < right._count; ++ii) {
            _first[ii] = right._first[ii];
        }
        SetCount(right._count);
    }

    void operator=(const std::initializer_list<T> &list) {
        if (list.size() > _count) {
            GrowTo(list.size());
        }
        const T *src = list.begin();
        const T *end = list.end();
        for (size_t ii = 0; src < end; ++ii, ++src) {
            _first[ii] = *src;
        }
        SetCount(list.size());
    }

    int32_t size32(void) const
    {
        return((int32_t)_count);
    }

    int32_t size(void) const
    {
        return((int32_t)_count);
    }

    size_t size64(void) const
    {
        return(_count);
    }

    void clear(void)
    {
        SetCount(0);
    }

    T *begin(void) const
    {
        return(_first);
    }

    T *end(void) const
    {
        return(_first + _count);
    }

    void reserve(size_t count) {
        GrowTo(count);
    }

    bool erase(size_t first, size_t last)
    {
        if (last > (size_t)_count) last = (size_t)_count;
        if (first >= last) {
            return(false);
        }
        size_t dst = first;
        size_t src = last;
        while (src < (size_t)_count) {
            _first[dst++] = _first[src++];
        }
        SetCount(_count);
        return(true);
    }

    void insert(size_t position, T value)
    {
        if (position >(size_t)_count) return;
        if (_count >= _allocated) {
            GrowTo(_count + 1);
        }
        size_t dst = (size_t)_count;
        while (dst > position) {
            _first[dst] = _first[dst - 1];
            --dst;
        }
        _first[position] = value;
        ++_count;
    }

    void insert_range(size_t position, size_t count, const T *values)
    {
        size_t  ii;

        if (position > (size_t)_count) position = (size_t)_count;
        if (_count + count > _allocated) {
            GrowTo(_count + count);
        }
        size_t dst = (size_t)_count + count - 1;
        size_t src = (size_t)_count - 1;
        for (ii = _count - position; ii; --ii) {
            _first[dst--] = _first[src--];
        }
        for (ii = 0; ii < count; ++ii) {
            _first[position + ii] = values[ii];
        }
        _count += count;
    }

    void append(const vect<T> &other)
    {
        size_t  ii;

        if (other._count <= 0) return;
        if (_count + other._count > _allocated) {
            GrowTo(_count + other._count);
        }
        for (ii = 0; ii < other._count; ++ii) {
            _first[_count + ii] = other._first[ii];
        }
        _count += other._count;
    }

    virtual void SetCount(size_t count) {};

protected:
    T       *_first;
    size_t	_count;
    size_t  _allocated;

    virtual void GrowTo(size_t itemscount) = 0;
};

// dynamic vector for objects with a constructor

template<class T>
class dvect : public vect<T> {
public:
    dvect() {
        this->_count = this->_allocated = 0;
        this->_first = nullptr;
    }

    // constructor with preallocated size
    dvect(size_t size) {
        this->_count = this->_allocated = size;
        this->_first = new T[size];
    }

    // constructor for initialized vector
    dvect(const std::initializer_list<T> &list) {
        this->_first = new T[list.size()];
        this->_count = this->_allocated = list.size();
        const T *src = list.begin();
        for (size_t ii = 0; ii < this->_count; ++ii) {
            this->_first[ii] = *src++;
        }
    }

    // to allocate a run time sized, initialized vector
    dvect(size_t size, const std::initializer_list<T> &list) {
        size_t tocopy = list.size();
        this->_count = this->_allocated = std::max(size, tocopy);
        this->_first = new T[this->_count];
        const T *src = list.begin();
        size_t ii = 0;
        while (ii < tocopy) {
            this->_first[ii++] = *src++;
        }
        --src;
        while (ii < this->_count) {
            this->_first[ii++] = *src;
        }
    }

    // copy inited
    dvect(const vect<T> &right) {
        this->_count = this->_allocated = right.size64();
        this->_first = new T[this->_count];
        for (size_t ii = 0; ii < this->_count; ++ii) {
            this->_first[ii] = right.begin()[ii];
        }
    }
    dvect(const dvect<T> &right) {
        this->_count = this->_allocated = right.size64();
        this->_first = new T[this->_count];
        for (size_t ii = 0; ii < this->_count; ++ii) {
            this->_first[ii] = right.begin()[ii];
        }
    }

    // to allocate a run time sized, copy-inited vector
    dvect(size_t size, const vect<T> &right) {
        size_t tocopy = right.size64();
        this->_count = this->_allocated = std::max(size, tocopy);
        this->_first = new T[this->_count];
        for (size_t ii = 0; ii < tocopy; ++ii) {
            this->_first[ii] = right.begin()[ii];
        }
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }
    void operator=(const dvect<T> &right) { vect<T>::operator=(right); }
    void operator=(const std::initializer_list<T> &list) { vect<T>::operator=(list); }

protected:
    void GrowTo(size_t itemscount)
    {
        // are we ok ?
        if (this->_allocated >= itemscount) return;

        // allocate a new buffer
        size_t newsize = this->_allocated;
        if (newsize == 0) newsize = 1;
        while (newsize < itemscount) newsize <<= 1;
        T *newone = new T[newsize];

        // copy the old values
        for (size_t ii = 0; ii < this->_count; ++ii) {
            newone[ii] = this->_first[ii];
        }

        // free old stuff (if owned !!)
        delete[] this->_first;

        this->_first = newone;
        this->_allocated = newsize;
    }
    virtual void SetCount(size_t count) { this->_count = count; };
};

// dynamic vector for plain old data types

template<class T>
class dpvect : public vect<T> {
public:
    dpvect() {
        this->_count = this->_allocated = 0;
        this->_first = nullptr;
    }

    dpvect(size_t size) {
        this->_count = this->_allocated = size;
        this->_first = new T[size];
        memset(this->_first, 0, sizeof(T) * this->_count);
    }

    // constructor for initialized vector
    dpvect(const std::initializer_list<T> &list) {
        this->_first = new T[list.size()];
        this->_count = this->_allocated = list.size();
        const T *src = list.begin();
        for (size_t ii = 0; ii < this->_count; ++ii) {
            this->_first[ii] = *src++;
        }
    }

    // to allocate a run time sized, list-initialized vector
    dpvect(size_t size, const std::initializer_list<T> &list) {

        // allocate
        size_t tocopy = list.size();
        this->_count = this->_allocated = std::max(size, tocopy);
        this->_first = new T[this->_count];

        // copy initialized filelds
        const T *src = list.begin();
        memcpy(this->_first, src, sizeof(T) * tocopy);

        // init the other with the last item from the list
        src += tocopy - 1;
        for (size_t ii = tocopy; ii < this->_count; ++ii) {
            this->_first[ii] = *src;
        }
    }

    // copy inited
    dpvect(const vect<T> &right) {
        this->_count = this->_allocated = right.size64();
        this->_first = new T[this->_count];
        memcpy(this->_first, right.begin(), sizeof(T) * this->_count);
    }
    dpvect(const dpvect<T> &right) {
        this->_count = this->_allocated = right.size64();
        this->_first = new T[this->_count];
        memcpy(this->_first, right.begin(), sizeof(T) * this->_count);
    }

    // to allocate a run time sized, copy-inited vector
    dpvect(size_t size, const vect<T> &right) {
        size_t tocopy = right.size64();
        this->_count = this->_allocated = std::max(size, tocopy);
        this->_first = new T[this->_count];
        memcpy(this->_first, right.begin(), sizeof(T) * tocopy);
        if (this->_count > tocopy) {
            memset(this->_first + tocopy, 0, sizeof(T) * (this->_count - tocopy));
        }
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }
    void operator=(const dpvect<T> &right) { vect<T>::operator=(right); }
    void operator=(const std::initializer_list<T> &list) { vect<T>::operator=(list); }

protected:
    void GrowTo(size_t itemscount)
    {
        // are we ok ?
        if (this->_allocated >= itemscount) return;

        // allocate a new buffer
        size_t newsize = this->_allocated;
        if (newsize == 0) newsize = 1;
        while (newsize < itemscount) newsize <<= 1;
        T *newone = new T[newsize];

        // copy the old values
        memcpy(newone, this->_first, this->_count * sizeof(T));

        // init the new ones.
        memset(newone + this->_count, 0, sizeof(T) * (newsize - this->_count));

        // free old stuff (if owned !!)
        delete[] this->_first;

        this->_first = newone;
        this->_allocated = newsize;
    }
    virtual void SetCount(size_t count) { this->_count = count; }
};

// static vector for objects with a constructor

template<class T, size_t len>
class svect : public vect<T> {
    T storage[len];

public:
    svect() {
        this->_count = len;
        this->_allocated = 0;
        this->_first = storage;
    }

    // constructor for initialized vector
    svect(const std::initializer_list<T> &list) {
        this->_allocated = 0;
        this->_first = storage;
        this->_count = len;
        const T *src = list.begin();
        const T *end = list.end();
        size_t ii = 0;
        while (ii < this->_count && src < end) {
            this->_first[ii++] = *src++;      
        }
        --src;
        while (ii < this->_count) {
            this->_first[ii++] = *src;
        }
    }

    // copy inited
    svect(const vect<T> &right) {
        this->_count = len;
        this->_allocated = 0;
        this->_first = storage;
        if (right.size64() > this->_count) {
            throw(std::out_of_range("array index"));
        }
        for (size_t ii = 0; ii < right.size64(); ++ii) {
            this->_first[ii] = right.begin()[ii];
        }
    }
    svect(const svect<T, len> &right) {
        this->_count = len;
        this->_allocated = 0;
        this->_first = storage;
        if (right.size64() > this->_count) {
            throw(std::out_of_range("array index"));
        }
        for (size_t ii = 0; ii < right.size64(); ++ii) {
            this->_first[ii] = right.begin()[ii];
        }
    }
    void operator=(const vect<T> &right) { vect<T>::operator=(right); }
    void operator=(const svect<T, len> &right) { vect<T>::operator=(right); }

protected:
    void GrowTo(size_t itemscount)
    {
        throw(std::out_of_range("static array index"));
    }
};

// static vector for plain old data types

template<class T, size_t len>
class spvect : public vect<T> {

    T storage[len];

public:
    spvect() {
        this->_count = len;
        this->_allocated = 0;
        this->_first = storage;
        memset(storage, 0, sizeof(storage));
    }

    // constructor for initialized vector
    spvect(const std::initializer_list<T> &list) {
        this->_allocated = 0;
        this->_first = storage;
        this->_count = len;
        const T *src = list.begin();
        const T *end = list.end();
        size_t ii = 0;
        for (; ii < this->_count && src < end; ++ii) {
            this->_first[ii] = *src++;
        }
        --src;
        while (ii < this->_count) {
            this->_first[ii++] = *src;
        }
    }

    // copy inited
    spvect(const vect<T> &right) {
        this->_count = len;
        this->_allocated = 0;
        this->_first = storage;
        size_t tocopy = right.size64();
        if (tocopy > this->_count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(this->_first, right.begin(), sizeof(T) * tocopy);
        if (this->_count > tocopy) {
            memset(this->_first + tocopy, 0, sizeof(T) * (this->_count - tocopy));
        }
    }
    spvect(const spvect<T, len> &right) {
        this->_count = len;
        this->_allocated = 0;
        this->_first = storage;
        size_t tocopy = right.size64();
        if (tocopy > this->_count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(this->_first, right.begin(), sizeof(T) * tocopy);
        if (this->_count > tocopy) {
            memset(this->_first + tocopy, 0, sizeof(T) * (this->_count - tocopy));
        }
    }

    // = operator, optimized
    void operator=(const vect<T> &right) {
        if (right.size64() > this->_count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(this->_first, right.begin(), sizeof(T) * right.size64());
    }
    void operator=(const spvect<T, len> &right) { 
        if (right.size64() > this->_count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(this->_first, right.begin(), sizeof(T) * right.size64());
    }

protected:
    void GrowTo(size_t itemscount)
    {
        throw(std::out_of_range("static array index"));
    }
};

// points to a portion of another vector.

template<class T>
class slice : public vect<T> {
public:
    // from a vector
    slice(const vect<T> &right, size_t first, size_t last) {
        if (first >= last) {
            throw(std::out_of_range("empty slice"));
        }
        if (last > right.size64()) {
            throw(std::out_of_range("slice exceeds the vector size"));
        }
        this->_count = last - first;
        this->_allocated = 0;
        this->_first = right.begin() + first;
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }
    void operator=(const slice<T> &right) { vect<T>::operator=(right); }

protected:
    void GrowTo(size_t itemscount)
    {
        throw(std::out_of_range("slice array index"));
    }
};

// points to the upper portion of another vector, can grow

template<class T>
class oslice : public vect<T> {
    vect<T>   *base;
public:
    // from a vector
    oslice(vect<T> &right, size_t first) {
        if (first > right.size64()) {
            throw(std::out_of_range("open slice exceeds the vector size"));
        }
        base = &right;
        size_t last = right.size64();
        this->_count = last - first;
        this->_allocated = 0;
        this->_first = right.begin() + first;
    }

    // from a vector
    oslice(const vect<T> &right, size_t first) {
        if (first > right.size64()) {
            throw(std::out_of_range("open slice exceeds the vector size"));
        }
        base = nullptr;
        size_t last = right.size64();
        this->_count = last - first;
        this->_allocated = 0;
        this->_first = right.begin() + first;
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }

protected:
    void GrowTo(size_t itemscount)
    {
        if (base == nullptr) {
            throw(std::out_of_range("slice array index"));
        }
        size_t first = this->_first - base->begin();
        base->reserve(itemscount + first);
        this->_first = base->begin() + first;
    }

    void SetCount(size_t count) {
        if (base != nullptr) {
            size_t first = this->_first - base->begin();
            base->SetCount(count + first);
            this->_count = base->size() - first;
        }
    }
};

}   // namespace

#endif