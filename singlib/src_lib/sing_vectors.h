#ifndef SING_VECTORS_H_
#define SING_VECTORS_H_

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
        _count = _allocated = 0;
        _first = nullptr;
    }

    // constructor with preallocated size
    dvect(size_t size) {
        _count = _allocated = size;
        _first = new T[size];
    }

    // constructor for initialized vector
    dvect(const std::initializer_list<T> &list) {
        _first = new T[list.size()];
        _count = _allocated = list.size();
        const T *src = list.begin();
        for (size_t ii = 0; ii < _count; ++ii) {
            _first[ii] = *src++;
        }
    }

    // to allocate a run time sized, initialized vector
    dvect(size_t size, const std::initializer_list<T> &list) {
        size_t tocopy = list.size();
        _count = _allocated = std::max(size, tocopy);
        _first = new T[_count];
        const T *src = list.begin();
        size_t ii = 0;
        while (ii < tocopy) {
            _first[ii++] = *src++;
        }
        --src;
        while (ii < _count) {
            _first[ii++] = *src;
        }
    }

    // copy inited
    dvect(const vect<T> &right) {
        _count = _allocated = right.size64();
        _first = new T[_count];
        for (size_t ii = 0; ii < _count; ++ii) {
            _first[ii] = right.begin()[ii];
        }
    }
    dvect(const dvect<T> &right) {
        _count = _allocated = right.size64();
        _first = new T[_count];
        for (size_t ii = 0; ii < _count; ++ii) {
            _first[ii] = right.begin()[ii];
        }
    }

    // to allocate a run time sized, copy-inited vector
    dvect(size_t size, const vect<T> &right) {
        size_t tocopy = right.size64();
        _count = _allocated = std::max(size, tocopy);
        _first = new T[_count];
        for (size_t ii = 0; ii < tocopy; ++ii) {
            _first[ii] = right.begin()[ii];
        }
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }
    void operator=(const dvect<T> &right) { vect<T>::operator=(right); }
    void operator=(const std::initializer_list<T> &list) { vect<T>::operator=(list); }

protected:
    void GrowTo(size_t itemscount)
    {
        // are we ok ?
        if (_allocated >= itemscount) return;

        // allocate a new buffer
        size_t newsize = _allocated;
        if (newsize == 0) newsize = 1;
        while (newsize < itemscount) newsize <<= 1;
        T *newone = new T[newsize];

        // copy the old values
        for (size_t ii = 0; ii < _count; ++ii) {
            newone[ii] = _first[ii];
        }

        // free old stuff (if owned !!)
        delete[] _first;

        _first = newone;
        _allocated = newsize;
    }
    virtual void SetCount(size_t count) { _count = count; };
};

// dynamic vector for plain old data types

template<class T>
class dpvect : public vect<T> {
public:
    dpvect() {
        _count = _allocated = 0;
        _first = nullptr;
    }

    dpvect(size_t size) {
        _count = _allocated = size;
        _first = new T[size];
        memset(_first, 0, sizeof(T) * _count);
    }

    // constructor for initialized vector
    dpvect(const std::initializer_list<T> &list) {
        _first = new T[list.size()];
        _count = _allocated = list.size();
        const T *src = list.begin();
        for (size_t ii = 0; ii < _count; ++ii) {
            _first[ii] = *src++;
        }
    }

    // to allocate a run time sized, list-initialized vector
    dpvect(size_t size, const std::initializer_list<T> &list) {

        // allocate
        size_t tocopy = list.size();
        _count = _allocated = std::max(size, tocopy);
        _first = new T[_count];

        // copy initialized filelds
        const T *src = list.begin();
        memcpy(_first, src, sizeof(T) * tocopy);

        // init the other with the last item from the list
        src += tocopy - 1;
        for (size_t ii = tocopy; ii < _count; ++ii) {
            _first[ii] = *src;
        }
    }

    // copy inited
    dpvect(const vect<T> &right) {
        _count = _allocated = right.size64();
        _first = new T[_count];
        memcpy(_first, right.begin(), sizeof(T) * _count);
    }
    dpvect(const dpvect<T> &right) {
        _count = _allocated = right.size64();
        _first = new T[_count];
        memcpy(_first, right.begin(), sizeof(T) * _count);
    }

    // to allocate a run time sized, copy-inited vector
    dpvect(size_t size, const vect<T> &right) {
        size_t tocopy = right.size64();
        _count = _allocated = std::max(size, tocopy);
        _first = new T[_count];
        memcpy(_first, right.begin(), sizeof(T) * tocopy);
        if (_count > tocopy) {
            memset(_first + tocopy, 0, sizeof(T) * (_count - tocopy));
        }
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }
    void operator=(const dpvect<T> &right) { vect<T>::operator=(right); }
    void operator=(const std::initializer_list<T> &list) { vect<T>::operator=(list); }

protected:
    void GrowTo(size_t itemscount)
    {
        // are we ok ?
        if (_allocated >= itemscount) return;

        // allocate a new buffer
        size_t newsize = _allocated;
        if (newsize == 0) newsize = 1;
        while (newsize < itemscount) newsize <<= 1;
        T *newone = new T[newsize];

        // copy the old values
        memcpy(newone, _first, _count * sizeof(T));

        // init the new ones.
        memset(newone + _count, 0, sizeof(T) * (newsize - _count));

        // free old stuff (if owned !!)
        delete[] _first;

        _first = newone;
        _allocated = newsize;
    }
    virtual void SetCount(size_t count) { _count = count; }
};

// static vector for objects with a constructor

template<class T, size_t len>
class svect : public vect<T> {
    T storage[len];

public:
    svect() {
        _count = len;
        _allocated = 0;
        _first = storage;
    }

    // constructor for initialized vector
    svect(const std::initializer_list<T> &list) {
        _allocated = 0;
        _first = storage;
        _count = len;
        const T *src = list.begin();
        const T *end = list.end();
        size_t ii = 0;
        while (ii < _count && src < end) {
            _first[ii++] = *src++;      
        }
        --src;
        while (ii < _count) {
            _first[ii++] = *src;
        }
    }

    // copy inited
    svect(const vect<T> &right) {
        _count = len;
        _allocated = 0;
        _first = storage;
        if (right.size64() > _count) {
            throw(std::out_of_range("array index"));
        }
        for (size_t ii = 0; ii < right.size64(); ++ii) {
            _first[ii] = right.begin()[ii];
        }
    }
    svect(const svect<T, len> &right) {
        _count = len;
        _allocated = 0;
        _first = storage;
        if (right.size64() > _count) {
            throw(std::out_of_range("array index"));
        }
        for (size_t ii = 0; ii < right.size64(); ++ii) {
            _first[ii] = right.begin()[ii];
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
        _count = len;
        _allocated = 0;
        _first = storage;
        memset(storage, 0, sizeof(storage));
    }

    // constructor for initialized vector
    spvect(const std::initializer_list<T> &list) {
        _allocated = 0;
        _first = storage;
        _count = len;
        const T *src = list.begin();
        const T *end = list.end();
        size_t ii = 0;
        for (; ii < _count && src < end; ++ii) {
            _first[ii] = *src++;
        }
        --src;
        while (ii < _count) {
            _first[ii++] = *src;
        }
    }

    // copy inited
    spvect(const vect<T> &right) {
        _count = len;
        _allocated = 0;
        _first = storage;
        size_t tocopy = right.size64();
        if (tocopy > _count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(_first, right.begin(), sizeof(T) * tocopy);
        if (_count > tocopy) {
            memset(_first + tocopy, 0, sizeof(T) * (_count - tocopy));
        }
    }
    spvect(const spvect<T, len> &right) {
        _count = len;
        _allocated = 0;
        _first = storage;
        size_t tocopy = right.size64();
        if (tocopy > _count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(_first, right.begin(), sizeof(T) * tocopy);
        if (_count > tocopy) {
            memset(_first + tocopy, 0, sizeof(T) * (_count - tocopy));
        }
    }

    // = operator, optimized
    void operator=(const vect<T> &right) {
        if (right.size64() > _count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(_first, right.begin(), sizeof(T) * right.size64());
    }
    void operator=(const spvect<T, len> &right) { 
        if (right.size64() > _count) {
            throw(std::out_of_range("array index"));
        }
        memcpy(_first, right.begin(), sizeof(T) * right.size64());
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
        _count = last - first;
        _allocated = 0;
        _first = right.begin() + first;
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
        _count = last - first;
        _allocated = 0;
        _first = right.begin() + first;
    }

    // from a vector
    oslice(const vect<T> &right, size_t first) {
        if (first > right.size64()) {
            throw(std::out_of_range("open slice exceeds the vector size"));
        }
        base = nullptr;
        size_t last = right.size64();
        _count = last - first;
        _allocated = 0;
        _first = right.begin() + first;
    }

    void operator=(const vect<T> &right) { vect<T>::operator=(right); }

protected:
    void GrowTo(size_t itemscount)
    {
        if (base == nullptr) {
            throw(std::out_of_range("slice array index"));
        }
        size_t first = _first - base->begin();
        base->reserve(itemscount + first);
        _first = base->begin() + first;
    }

    void SetCount(size_t count) {
        if (base != nullptr) {
            size_t first = _first - base->begin();
            base->SetCount(count + first);
            _count = base->size() - first;
        }
    }
};

#define SING_SIZE_T size_t

template<class T, bool is_pod = false>
class vector {
    T		    *_first;
    SING_SIZE_T	_count;
    SING_SIZE_T _allocated;

    void GrowTo(SING_SIZE_T itemscount)
    {
        SING_SIZE_T	newsize;
        T	        *newone;

        //ASSERT(itemscount < 0x40000000);
        if (_allocated >= itemscount) return;
        if (_allocated == 0 && _first != nullptr) {
            // is a slice: you can't grow it !!
            throw(std::out_of_range("array index / trying to grow a static array or a sub-array"));
        }
        newsize = _allocated;
        if (newsize == 0) newsize = 1;
        while (newsize < itemscount) newsize <<= 1;
        newone = new T[newsize];
        //ASSERT(newone != nullptr);

        // copy the old values
        for (SING_SIZE_T ii = 0; ii < _count; ++ii) {
            newone[ii] = _first[ii];
        }

        // init the new ones if they have not a constructor.
        if (is_pod) {
            memset(&newone[_count], 0, sizeof(T) * (newsize - _count));
        }

        // free old stuff (if owned !!)
        if (_allocated != 0) {
            delete[] _first;
        }
        _first = newone;
        _allocated = newsize;
    }

public:

    //
    // CONSTRUCTORS
    //

    // default
    vector() : _count(0), _allocated(0), _first(nullptr) {}

    // copy
    vector(const vector<T> &right) : _count(0), _allocated(0), _first(nullptr) {
        *this = right;
    }

    // to create a slice from a const
    vector(const vector<T> &right, SING_SIZE_T first, SING_SIZE_T last) : _count(last - first),
        _allocated(0),
        _first(right._first + first) {
        // allocate elements in right if needed
        if (last >= right->_count) {
            throw(std::out_of_range("slice index"));
        }
    }

    // to create a slice from a static vector
    vector(T *storage, SING_SIZE_T first, SING_SIZE_T last) : _count(last - first),
        _allocated(0),
        _first(storage + first) {
    }

    // to allocate a run time sized vector
    vector(SING_SIZE_T size) : _count(0), _allocated(0), _first(nullptr) {
        GrowTo(size);
        _count = size;
    }

    // initialized vector
    vector(const std::initializer_list<T> &list) : _count(0), _allocated(0), _first(nullptr) {
        GrowTo(list.size());
        _count = list.size();
        const T *src = list.begin();
        for (SING_SIZE_T ii = 0; ii < _count; ++ii) {
            _first[ii] = *src++;
        }
    }

    // to allocate a run time sized, initialized vector
    vector(SING_SIZE_T size, const std::initializer_list<T> &list) : _count(0), _allocated(0), _first(nullptr) {
        GrowTo(size);
        _count = size;
        const T *src = list.begin();
        SING_SIZE_T tocopy = list.size() - 1;
        if (tocopy > _count) tocopy = _count;
        SING_SIZE_T ii = 0;
        while (ii < tocopy) {
            _first[ii++] = *src++;
        }
        while (ii < _count) {
            _first[ii++] = *src;
        }
    }

    ~vector() {
        if (_allocated != 0 && _first != nullptr) {
            delete[] _first;
        }
    }

    // [] operator
    T& operator[](SING_SIZE_T i)
    {
        if (i >= _count) {
            if (i == _count) {
                if (_count >= _allocated) {
                    GrowTo(_count + 1);
                    _count++;
                } else {
                    _first[_count++].~T();
                    new(&_first[_count++]) T;
                }
            } else {
                throw(std::out_of_range("array index"));
            }
        }
        return _first[i];
    }

    const T& operator[](SING_SIZE_T i) const
    {
        if (i >= _count) {
            throw(std::out_of_range("array index"));
        }
        return _first[i];
    }

    T& at(SING_SIZE_T i)
    {
        return _first[i];
    }

    bool isslice(void) {
        return(_allocated == 0 && _first != nullptr);
    }

    // = operator
    vector<T>& operator=(const vector<T> &right) {
        if (isslice()) {
            if (right._count != _count) {
                throw(std::out_of_range("array index"));
            }
        } else {
            if (right._count > _allocated) {
                GrowTo(right._count);
            }
        }
        for (SING_SIZE_T ii = 0; ii < right._count; ++ii) {
            _first[ii] = right._first[ii];
        }
        _count = right._count;
        return *this;
    }

    vector<T>& operator=(const std::initializer_list<T> &list) {
        if (isslice()) {
            if (list.size() != _count) {
                throw(std::out_of_range("array index"));
            }
        } else {
            if (list.size() > _allocated) {
                GrowTo(list.size());
            }
        }
        _count = list.size();
        const T *src = list.begin();
        for (SING_SIZE_T ii = 0; ii < _count; ++ii) {
            _first[ii] = *src++;
        }
        return *this;
    }

    void push_back(const T& value)
    {
        if (_count >= _allocated) {
            T   saver = value;  // if the reference is from this vector may become invalid
                                // (BE SURE T HAS AN APPROPRIATE COPY CONSTRUCTOR !!!)
            GrowTo(_count + 1);
            _first[_count++] = saver;
        } else {
            _first[_count++] = value;
        }
    }

    // push a randomly inited item
    void grow_one_element(void)
    {
        if (_count >= _allocated) {
            GrowTo(_count + 1);
        }
        _count++;
    }

    void pop_back(void)
    {
        //ASSERT(_count > 0);
        if (_count > 0) --_count;
    }

    void reserve(SING_SIZE_T size)
    {
        if (_allocated < size) {
            GrowTo(size);
        }
    }

    SING_SIZE_T size(void) const
    {
        return((SING_SIZE_T)_count);
    }

    void clear(void)
    {
        _count = 0;
    }

    bool erase(SING_SIZE_T first, SING_SIZE_T last)
    {
        //ASSERT(first < (DWORD)_count);
        //ASSERT(last < (DWORD)_count);
        //ASSERT(last >= first);
        if (last > (SING_SIZE_T)_count) last = (SING_SIZE_T)_count;
        if (first >= last) {
            return(false);
        }
        SING_SIZE_T dst = first;
        SING_SIZE_T src = last;
        while (src < (SING_SIZE_T)_count) {
            _first[dst++] = _first[src++];
        }
        _count = dst;
        return(true);
    }

    T *begin(void) const
    {
        return(_first);
    }

    T *end(void) const
    {
        return(_first + _count);
    }

    void insert(SING_SIZE_T position, T value)
    {
        if (position > (SING_SIZE_T)_count) return;
        if (_count >= _allocated) {
            GrowTo(_count + 1);
        }
        SING_SIZE_T dst = (SING_SIZE_T)_count;
        while (dst > position) {
            _first[dst] = _first[dst - 1];
            --dst;
        }
        _first[position] = value;
        ++_count;
    }

    void insert_range(SING_SIZE_T position, SING_SIZE_T count, const T *values)
    {
        SING_SIZE_T  ii;

        if (position > (SING_SIZE_T)_count) position = (SING_SIZE_T)_count;
        if (_count + count > _allocated) {
            GrowTo(_count + count);
        }
        SING_SIZE_T dst = (SING_SIZE_T)_count + count - 1;
        SING_SIZE_T src = (SING_SIZE_T)_count - 1;
        for (ii = _count - position; ii; --ii) {
            _first[dst--] = _first[src--];
        }
        for (ii = 0; ii < count; ++ii) {
            _first[position + ii] = values[ii];
        }
        _count += count;
    }

    void append(const vector<T> &other)
    {
        SING_SIZE_T  ii;

        if (other._count <= 0) return;
        if (_count + other._count > _allocated) {
            GrowTo(_count + other._count);
        }
        for (ii = 0; ii < other._count; ++ii) {
            _first[_count + ii] = other._first[ii];
        }
        _count += other._count;
    }

    bool empty(void)
    {
        return(_count == 0);
    }

    bool isequal(const vector<T> &other)
    {
        if (_count != other._count) return(false);
        for (SING_SIZE_T ii = 0; ii < (SING_SIZE_T)_count; ++ii) {
            if (_first[ii] != other._first[ii]) return(false);
        }
        return(true);
    }

    static int compare(const void *first, const void *second)
    {
        const T *ff, *ss;

        ff = (const T*)first;
        ss = (const T*)second;

        if (*ss < *ff) {
            return(1);
        } else if (*ff < *ss) {
            return(-1);
        } else {
            return(0);
        }
    }

    // beware !! can use only for simple types (i.e.
    void sort_shallow_copy(SING_SIZE_T start, SING_SIZE_T end)
    {
        if (start > end || start > (SING_SIZE_T)_count || end > (SING_SIZE_T)_count) return;
        qsort(_first + start, end - start, sizeof(T), &compare);
    }

    void set_union(const vector<T> &arg0, const vector<T> &arg1)
    {
        SING_SIZE_T  src0, src1;
        T       *reference, *candidate;

        // empty source -> empty dst
        _count = 0;
        if (arg0._count == 0 && arg1._count == 0) return;

        // try to avoid resizing too often
        if (_allocated < arg0._count || _allocated < arg1._count) {
            if (arg0._count > arg1._count) {
                GrowTo(arg0._count);
            } else {
                GrowTo(arg1._count);
            }
        }

        // first copy sets the reference value
        src0 = src1 = 0;
        if (arg0._count > 0 && (arg1._count == 0 || arg0._first[0] < arg1._first[0])) {
            _first[0] = arg0._first[0];
            src0 = 1;
        } else {
            _first[0] = arg1._first[0];
            src1 = 1;
        }
        reference = _first;
        _count = 1;

        // at each turn select the more appropriate source and, if applicable, copy
        while (src0 < (SING_SIZE_T)arg0._count || src1 < (SING_SIZE_T)arg1._count) {
            if (src0 < (SING_SIZE_T)arg0._count && (src1 >= (SING_SIZE_T)arg1._count ||
                arg0._first[src0] < arg1._first[src1])) {
                candidate = arg0._first + src0++;
            } else {
                candidate = arg1._first + src1++;
            }
            if (*reference < *candidate) {
                if (_count >= _allocated) {
                    GrowTo(_count + 1);
                }
                _first[_count++] = *candidate;
                ++reference;
            }
        }
    }

    void set_difference(const vector<T> &arg0, const vector<T> &arg1)
    {
        T       *candidate;
        SING_SIZE_T  scan = 0;

        // consider all the arg0 items
        _count = 0;
        candidate = arg0._first;
        for (SING_SIZE_T ii = 0; ii < (SING_SIZE_T)arg0._count; ++ii) {

            // find first inhibiting value greater or equal to the candidate in arg1
            while (scan < (SING_SIZE_T)arg1._count && arg1._first[scan] < *candidate) {
                ++scan;
            }

            // if the arg1 value dont exist or dont matches (is greater) -> push back
            if (scan == (SING_SIZE_T)arg1._count || *candidate < arg1._first[scan]) {
                if (_count >= _allocated) {
                    GrowTo(_count + 1);
                }
                _first[_count++] = *candidate;
            }
            ++candidate;
        }
    }
};

}   // namespace

#endif