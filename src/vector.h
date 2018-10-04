#ifndef __URGHVECTOR_H_
#define __URGHVECTOR_H_

typedef unsigned int URGH_SIZE_T;    // to ease the build of 64 bit apps - limits the vectors to 4Gelements.

#include <sys/types.h>

#ifndef NULL
	#define NULL	0
#endif

namespace SingNames {

template<class T>
class vector {
    T		*_first;
    int		_count;	
    int		_allocated;

	void GrowTo(int itemscount)
	{
		int	newsize;
		T	*newone;

		//ASSERT(itemscount < 0x40000000);
		if (_allocated >= itemscount) return;	
		newsize = _allocated;
		if (newsize == 0) newsize = 1;
		while (newsize < itemscount) newsize <<= 1;
		newone = new T[newsize];
		//ASSERT(newone != NULL);
		for (int ii = 0; ii < _count; ++ii) {
			newone[ii] = _first[ii];
		}
		if (_allocated != 0) {
			delete[] _first;
		}
		_first = newone;
		_allocated = newsize;
	}

public:
    vector() {_count = 0; _allocated = 0;_first = NULL;}
    vector(const vector<T> &right) {
        _count = 0; _allocated = 0;_first = NULL;
        *this=right;
    }
    ~vector() {
        if (_allocated != 0) {
            delete[] _first;
        }
    }

	// [] operator
    T& operator[](URGH_SIZE_T i)
	{
		return _first[i];
	}

    const T& operator[](URGH_SIZE_T i) const
	{
		return _first[i];
	}

	T& at(URGH_SIZE_T i) 
	{
		return _first[i];
	}

	// = operator
    vector<T>& operator=(const vector<T> &right) {
		if (right._count > _allocated) {
			GrowTo(right._count);
		}
		for (int ii = 0; ii < right._count; ++ii) {
			_first[ii] = right._first[ii];
		}
		_count = right._count;
        return *this;
    }
    
    void push_back(const T& value)
	{
		if (_count >= _allocated) {
            T   saver = value;  // if the reference is from this vector may become invalid
                                // (BE SURE T HAS AN APPROPRIATE COPY CONSTRUCTOR !!!)
			GrowTo(_count+1);
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

	void reserve(URGH_SIZE_T size)
	{
		if ((URGH_SIZE_T)_allocated < size) {
			GrowTo((int)size);
		}
	}

	URGH_SIZE_T size(void) const
	{
		return((URGH_SIZE_T)_count);
	}

	void clear(void)
	{
		_count = 0;
	}

	bool erase(URGH_SIZE_T first, URGH_SIZE_T last)
	{
		//ASSERT(first < (DWORD)_count);
		//ASSERT(last < (DWORD)_count);
		//ASSERT(last >= first);
        if (last > (URGH_SIZE_T)_count) last = (URGH_SIZE_T)_count;
        if (first >= last) {
            return(false);
        }
		URGH_SIZE_T dst = first;
		URGH_SIZE_T src = last;
		while (src < (URGH_SIZE_T)_count) {
			_first[dst++] = _first[src++];
		}
		_count = (int)dst;
        return(true);
	}
	
	URGH_SIZE_T begin(void) const
	{
		return(0);
	}
	
	URGH_SIZE_T end(void) const
	{
		return((URGH_SIZE_T)_count);
	}

	void insert(URGH_SIZE_T position, T value)
	{        
		if (position > (URGH_SIZE_T)_count) return;
		if (_count >= _allocated) {
			GrowTo(_count+1);
		}
		URGH_SIZE_T dst = (URGH_SIZE_T)_count;
		while(dst > position) {
			_first[dst] = _first[dst-1];
			--dst;
		}
		_first[position] = value;
        ++_count;
	}

	void insert_range(URGH_SIZE_T position, URGH_SIZE_T count, const T *values)
	{  
        URGH_SIZE_T  ii;

		if (position > (URGH_SIZE_T)_count) position = (URGH_SIZE_T)_count;
		if (_count + (int)count > _allocated) {
			GrowTo(_count + count);
		}
		URGH_SIZE_T dst = (URGH_SIZE_T)_count + count - 1;
        URGH_SIZE_T src = (URGH_SIZE_T)_count - 1; 
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
        int  ii;

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
        for (URGH_SIZE_T ii = 0; ii < (URGH_SIZE_T)_count; ++ii) {
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
    void sort_shallow_copy(URGH_SIZE_T start, URGH_SIZE_T end)
    {
        if (start > end || start > (URGH_SIZE_T)_count || end > (URGH_SIZE_T)_count) return;
        qsort(_first + start, end - start, sizeof(T), &compare);
    }

    void set_union(const vector<T> &arg0, const vector<T> &arg1)
    {
        URGH_SIZE_T  src0, src1;
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
        while (src0 < (URGH_SIZE_T)arg0._count || src1 < (URGH_SIZE_T)arg1._count) {
            if (src0 < (URGH_SIZE_T)arg0._count && (src1 >= (URGH_SIZE_T)arg1._count || 
                                               arg0._first[src0] < arg1._first[src1])) {
                candidate = arg0._first + src0++;
            } else {
                candidate = arg1._first + src1++;
            }
            if (*reference < *candidate) {
                if (_count >= _allocated) {
	                GrowTo(_count+1);
                }
                _first[_count++] = *candidate;
                ++reference;
           }
        }
    }

    void set_difference(const vector<T> &arg0, const vector<T> &arg1)
    {
        T       *candidate;
        URGH_SIZE_T  scan = 0;

        // consider all the arg0 items
        _count = 0;
        candidate = arg0._first;
        for (URGH_SIZE_T ii = 0; ii < (URGH_SIZE_T)arg0._count; ++ii) {

            // find first inhibiting value greater or equal to the candidate in arg1
            while (scan < (URGH_SIZE_T)arg1._count && arg1._first[scan] < *candidate) {
                ++scan;
            }

            // if the arg1 value dont exist or dont matches (is greater) -> push back
            if (scan == (URGH_SIZE_T)arg1._count || *candidate < arg1._first[scan]) {
                if (_count >= _allocated) {
	                GrowTo(_count+1);
                }
                _first[_count++] = *candidate;
            }
            ++candidate;
        }
    }

};

}
#endif