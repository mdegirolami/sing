#pragma once

namespace sing {

template<class T, size_t N>
class array {
public:
    // constructor 
    array() {}

    array(const std::initializer_list<T> &list) {
        *this = list;
    }

    array(const array<T, N> &src) {
        *this = src;
    }

    // operators
    void operator=(const std::initializer_list<T> &list) {
        size_t ii;

        if (list.size() < 1) return;
        const T *src = list.begin();
        for (ii = 0; ii < list.size() && ii < N; ++ii) {
            data[ii] = *src++;
        }
        --src;
        for (; ii < N; ++ii) {
            data[ii] = *src;
        }
    }

    array<T, N> &operator=(const array<T, N> &src) {
        for (size_t ii = 0; ii < N; ++ii) {
            data[ii] = src.data[ii];
        }
        return(*this);
    }

    bool operator==(const array<T, N> &right) const
    {
        for (size_t ii = 0; ii < N; ++ii) {
            if (data[ii] != right.data[ii]) {
                return(false);
            }
        }
        return(true);
    }

    bool operator!=(const array<T, N> &right) const
    {
        for (size_t ii = 0; ii < N; ++ii) {
            if (data[ii] != right.data[ii]) {
                return(true);
            }
        }
        return(false);
    }

    T& operator[](size_t i)
    {
        return(data[i]);
    }

    const T& operator[](size_t i) const
    {
        return(data[i]);
    }

    T& at(size_t i)
    {
        if (i >= N) {
            throw(std::out_of_range("array index"));
        }
        return(data[i]);
    }

    const T& at(size_t i) const
    {
        if (i >= N) {
            throw(std::out_of_range("array index"));
        }
        return(data[i]);
    }

    // support to builtins
    int32_t size(void) const
    {
        return((int32_t)N);
    }

    int64_t lsize(void) const
    {
        return((int64_t)N);
    }

    bool empty(void) const
    {
        return(false);
    }

    T& back(void)
    {
        return(data[N-1]);
    }

    const T& back(void) const
    {
        return(data[N-1]);
    }

    // support to iterations
    T *begin(void) 
    {
        return(data);
    }

    T *end(void)
    {
        return(data + N);
    }

    const T *begin(void) const
    {
        return(data);
    }

    const T *end(void) const
    {
        return(data + N);
    }

    // conversion (need to init a vector with an array)
    operator std::vector<T>() const 
    {
        return(std::vector<T>(begin(), end()));
    }

private:
    T data[N];
};

} // namespace
