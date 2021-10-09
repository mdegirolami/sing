#pragma once

#include <stdint.h>
#include <string.h>
#include <vector>
#include <complex>
#include <string>
#include <memory>
#include "sing_map.h"
#include "sing_arrays.h"

namespace sing {

    template<class T>
    inline T pow2(T base) {
        return(base * base);
    }

    //
    // all uint to int comparisons
    //
    inline bool ismore(uint64_t op1, int64_t op2) {
        return(op2 < 0 || op1 > (uint64_t)op2);
    }
    inline bool isless(uint64_t op1, int64_t op2) {
        return(op2 > 0 && op1 < (uint64_t)op2);
    }
    inline bool ismore_eq(uint64_t op1, int64_t op2) {
        return(op2 < 0 || op1 >= (uint64_t)op2);
    }
    inline bool isless_eq(uint64_t op1, int64_t op2) {
        return(op2 >= 0 && op1 <= (uint64_t)op2);
    }
    inline bool iseq(uint64_t op1, int64_t op2) {
        return(op2 >= 0 && op1 == (uint64_t)op2);
    }

    inline bool ismore(uint64_t op1, int32_t op2) {
        return(op2 < 0 || op1 >(uint64_t)op2);
    }
    inline bool isless(uint64_t op1, int32_t op2) {
        return(op2 > 0 && op1 < (uint64_t)op2);
    }
    inline bool ismore_eq(uint64_t op1, int32_t op2) {
        return(op2 < 0 || op1 >= (uint64_t)op2);
    }
    inline bool isless_eq(uint64_t op1, int32_t op2) {
        return(op2 >= 0 && op1 <= (uint64_t)op2);
    }
    inline bool iseq(uint64_t op1, int32_t op2) {
        return(op2 >= 0 && op1 == (uint64_t)op2);
    }

    inline bool ismore(uint32_t op1, int32_t op2) {
        return(op2 < 0 || op1 >(uint32_t)op2);
    }
    inline bool isless(uint32_t op1, int32_t op2) {
        return(op2 > 0 && op1 < (uint32_t)op2);
    }
    inline bool ismore_eq(uint32_t op1, int32_t op2) {
        return(op2 < 0 || op1 >= (uint32_t)op2);
    }
    inline bool isless_eq(uint32_t op1, int32_t op2) {
        return(op2 >= 0 && op1 <= (uint32_t)op2);
    }
    inline bool iseq(uint32_t op1, int32_t op2) {
        return(op2 >= 0 && op1 == (uint32_t)op2);
    }

    template<class T, class T2>
    T pow(T base, T2 exp) {
        if (base == 0) {
            if (exp == 0) {
                throw(std::domain_error("result of 0^0 is undefined"));
            }
            if (exp <= 0) {
                throw(std::overflow_error("result of 0^(anything negative) is infinite"));
            }
            return(0);
        }
        if (base < 0) {         // so if T is unsigned this is never executed
            if (base == -1) {   // this would be true for all-1s unsigned values
                return((exp & 1) == 0 ? 1 : -1);
            }
        }
        if (base == 1 || exp == 0) {
            return(1);
        }
        if (exp < 0) {
            return(0);
        } else if (exp == 1) {
            return(base);
        } else if (exp > 63) {
            throw(std::overflow_error("result of exponentiation overflows"));
        }

        T result = 1;
        T cur_power = base;
        while (exp > 0) {
            if (exp & 1) {
                result *= cur_power;
            }
            cur_power *= cur_power;
            exp >>= 1;
        }
        return(result);
    }

    inline std::complex<float> c_d2f(std::complex<double> in) {
        return(std::complex<float>((float)in.real(), (float)in.imag()));
    }

    inline std::complex<double> c_f2d(std::complex<float> in) {
        return(std::complex<double>(in.real(), in.imag()));
    }

    int64_t string2int(const char *instring);
    uint64_t string2uint(const char *instring);
    double string2double(const char *instring);
    std::complex<float> string2complex64(const char *instring);
    std::complex<double> string2complex128(const char *instring);

    // std::string stuff
    inline int64_t string2int(const std::string &instring) { return(string2int(instring.c_str())); }
    inline uint64_t string2uint(const std::string &instring) { return(string2uint(instring.c_str())); }
    inline double string2double(const std::string &instring) { return(string2double(instring.c_str())); }
    inline std::complex<float> string2complex64(const std::string &instring) { return(string2complex64(instring.c_str())); }
    inline std::complex<double> string2complex128(const std::string &instring) { return(string2complex128(instring.c_str())); }

    std::string to_string(std::complex<float> value);
    std::string to_string(std::complex<double> value);
    std::string to_string(bool value);

    std::string s_format(const char *fmt, ...);

    template<class T>
    inline int32_t sgn(T value)
    {
        if (value > 0) return(1);
        if (value < 0) return(-1);
        return(0);
    }

    template<class T>
    inline T abs(T value)
    {
        if (value >= 0) return(value);
        return(-value);
    }

    inline float exp10(float value)
    {
        return((float)exp(value * 2.30258509299f));
    }

    inline double exp10(double value)
    {
        return(exp(value * 2.30258509299));
    }

    template<class T, size_t N>
    inline void copy_array_to_vec(std::vector<T> &dst, const sing::array<T, N> &source)
    {
        dst.clear();
        dst.reserve(source.size());
        for (auto &element : source) {
            dst.push_back(element);
        }
    }

    template<class T, size_t N>
    inline bool iseq(const std::vector<T> &it0, const sing::array<T, N> &it1)
    {
        return(it0.size() == it1.size() && std::equal(it0.begin(), it0.end(), it1.begin()));
    }

    template<class T, size_t N>
    inline bool iseq(const sing::array<T, N> &it0, const std::vector<T> &it1)
    {
        return(it0.size() == it1.size() && std::equal(it0.begin(), it0.end(), it1.begin()));
    }

    template<class T>
    inline void insert(std::vector<T> &dst, int64_t idx, int64_t count, const T &value)
    {
        if (idx < 0) return;
        if (idx > dst.size()) idx = dst.size();
        dst.insert(dst.begin() + idx, count, value);
    }

    inline void insert(std::vector<std::string> &dst, int64_t idx, int64_t count, const char *value)
    {
        if (idx < 0) return;
        if (idx > dst.size()) idx = dst.size();
        dst.insert(dst.begin() + idx, count, value);
    }

    template<class T>
    inline void erase(std::vector<T> &dst, int64_t idx, int64_t top)
    {
        if (idx < 0 || idx >= dst.size() || top <= idx) return;
        if (top > dst.size()) top = dst.size();
        dst.erase(dst.begin() + idx, dst.begin() + top);
    }

    template<class T>
    inline void insert_v(std::vector<T> &dst, int64_t idx, const std::vector<T> &src)
    {
        if (idx < 0) return;
        if (idx > dst.size()) idx = dst.size();
        dst.insert(dst.begin() + idx, src.begin(), src.end());
    }

    template<class T>
    inline void append(std::vector<T> &dst, const std::vector<T> &src)
    {
        dst.insert(dst.end(), src.begin(), src.end());
    }
    
}   // namespace
/*
// from _mingw.h
#undef MINGW_HAS_SECURE_API
#undef USE___UUIDOF
#undef UNALIGNED
#undef NONAMELESSUNION
#undef DUMMYUNIONNAME 
#undef DUMMYUNIONNAME1
#undef DUMMYUNIONNAME2
#undef DUMMYUNIONNAME3
#undef DUMMYUNIONNAME4
#undef DUMMYUNIONNAME5
#undef DUMMYUNIONNAME6
#undef DUMMYUNIONNAME7
#undef DUMMYUNIONNAME8
#undef DUMMYUNIONNAME9
#undef MINGW_SDK_INIT
#undef NULL

// string.h
#undef strncasecmp
#undef strcasecmp
#undef wcswcs

// os_defines.h
#undef NOMINMAX
*/