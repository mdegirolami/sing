#include <type_traits>
#include "sing.h"

bool test_add(void);
bool test_sub(void);
bool test_mul(void);
bool test_shr(void);
bool test_shl(void);

template<class T>
inline bool addt(T op1, T op2) {
    T result = op1 + op2;
    if (std::is_unsigned<T>::value) {
        if (result < op1 || result < op2) {
            throw(std::overflow_error("integer operation overflows"));  
        }
    } else {
        if ((op1 ^ op2) >= 0 && (result ^ op1) < 0) {
            throw(std::overflow_error("integer operation overflows"));  
        }
    }
    return(result);
}

bool test_overflows(void)
{
    // just check which function gets called
    int8_t      op8 = 100;
    uint16_t    op16 = 110;
    std::vector<double> vd = {0.0f, 0.0f};
    sing::array<double, 2> ad = {0.0f, 0.0f};

    double  vv = 1e40;
    int pippo;

    pippo = vv;

    try {
        vv = std::pow(0.0f, -2);

        vv = acos(2.0f);
        vv = sqrt(-1.0f);
        //vv = ad.at(2);
        vv = vd.at(2);
    } catch(std::exception) {
        return(false);
    }

    //addt(op8, op8);
    sing::add(op8, op16);
    if (!test_add()) return(false);
    if (!test_sub()) return(false);
    if (!test_mul()) return(false);
    return(true);
}

bool test_add(void)
{
    int32_t op32a = 0x7fffffff;
    uint32_t uop32a = 0xffffffff;
    int64_t op64a = ((uint64_t)1 << 63) - 1;
    uint64_t uop64a = (int64_t)0 - 1;

    // not throwing
    try {

        // add
        sing::add(op32a, 0);
        sing::add((op32a + 2), -1);
        sing::add(op32a, -1);
        sing::add(-1, op32a);

        sing::add(op64a, 0LL);
        sing::add((op64a + 2), -1LL);
        sing::add(op64a, -1LL);
        sing::add(-1LL, op64a);

        // unsigned add
        sing::add(uop32a, (uint32_t)0);
        sing::add(uop32a >> 1, (uint32_t)1);

        sing::add(uop64a, (uint64_t)0);
        sing::add(uop64a >> 1, (uint64_t)1);
    } catch (std::overflow_error) {
        return(false);
    }

    int exceptions = 0;

    try {
        sing::add(op32a, 1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::add((op32a + 1), -1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::add(uop32a, (uint32_t)1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::add(op64a, 1LL);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::add((op64a + 1), -1LL);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::add(uop64a, (uint64_t)1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    return(exceptions == 0);
}

bool test_sub(void)
{
    int32_t op32a = 0x7fffffff;
    uint32_t uop32a = 0xffffffff;
    int64_t op64a = ((uint64_t)1 << 63) - 1;
    uint64_t uop64a = (int64_t)0 - 1;

    // not throwing
    try {

        // sub
        sing::sub(op32a - 1, -1);
        sing::sub((op32a + 2), 1);
        sing::sub(op32a, 1);
        sing::sub(0, op32a);

        sing::sub(op64a - 1, -1LL);
        sing::sub((op64a + 2), 1LL);
        sing::sub(op64a, 1LL);
        sing::sub(0LL, op64a);

        // unsigned sub
        sing::sub(uop32a, uop32a);
        sing::sub((uop32a >> 1) + 1, (uint32_t)1);

        sing::sub(uop64a, uop64a);
        sing::sub((uop64a >> 1) + 1, (uint64_t)1);

    } catch (std::overflow_error) {
        return(false);
    }

    int exceptions = 0;

    try {
        sing::sub(op32a, -1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::sub((op32a + 1), 1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::sub(uop32a - 1, uop32a);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::sub(op64a, -1LL);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::sub((op64a + 1), 1LL);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::sub(uop64a - 1, uop64a);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    return(exceptions == 0);
}

bool test_mul(void)
{
    int32_t op32a = 1 << 15;
    int32_t op32b = 1 << 16;
    int64_t op64a = 1LL << 31;
    int64_t op64b = 1LL << 32;

    // not throwing
    try {

        // mul
        sing::mul(op32a, op32b - 1);
        sing::mul(-op32a, op32b);

        sing::mul(op64a, op64b - 1);
        sing::mul(-op64a, op64b);

        // unsigned mul
        sing::mul((uint32_t)op32b, (uint32_t)op32b - 1);
        sing::mul((uint64_t)op64b, (uint64_t)op64b - 1);
    } catch (std::overflow_error) {
        return(false);
    }

    int exceptions = 0;

    try {
        sing::mul(op32a, op32b);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::mul(-op32a, op32b + 1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::mul(op64a, op64b);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::mul(-op64a, op64b + 1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::mul((uint32_t)op32b, (uint32_t)op32b);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::mul((uint64_t)op64b, (uint64_t)op64b);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    return(exceptions == 0);
}
/*
bool test_div(void)
{
    int32_t op32a = 1 << 31;
    int64_t op64a = 1LL << 63;

    // not throwing
    try {

        // div
        sing::div(op32a, 1);
        sing::div(op64a, 1LL);
    } catch (std::overflow_error) {
        return(false);
    }

    int exceptions = 0;

    try {
        sing::div(op32a, -1);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    try {
        sing::div(op64a, -1LL);
    } catch (std::overflow_error) {
        ++exceptions;
    }
    --exceptions;

    return(exceptions == 0);
}
*/