#include "sing.h"
#include <complex>

// int32_t values 
#define MAX_INT32   0x7fffffff          // 2147483647
#define VAL2EXP31   0x80000000          // 2147483648
#define MAX_UINT32  0xffffffff          // 4294967295

// uint64_t values
#define MAX_INT64   0x7fffffffffffffff  // 9223372036854775807
#define VAL2EXP63   0x8000000000000000  // 9223372036854775808
#define MAX_UINT64  0xffffffffffffffff  // 18446744073709551615

void CheckLiterals(void) {

    // must apply all the require conversion to:
    // - get the right result
    // - avoid warnings

    // rule 1: getting rid of uint32: numbers between 2^31 and 2^32 are postfixed with ll.

    // - operator:
    // if input is int32, -VAL2EXP31, cast to int64 before the operation
    // if input is int64, -VAL2EXP63, cast to uint64 after the operation -> NO IN LATEST VERSION
    // if input is uint64, cast to int64 before the operation (we suppose its value is legal)
    int64_t aa = -(-MAX_INT32-1);           // ko
    int64_t bb = -(-MAX_INT64 - 1);
    int64_t cc = (-VAL2EXP63) / 10;

    aa = -(int64_t)(-MAX_INT32 - 1);            // ok
    bb = ((uint64_t)-(-MAX_INT64 - 1)) / 10;    // divide by 10 to reduce to the int64_t range.
    cc = (-(int64_t)VAL2EXP63) / 10;

    // limits values which should give no problem:
    aa = -MAX_INT32;
    bb = -(-MAX_INT32);
    cc = -MAX_INT64;
    int64_t dd = -(-MAX_INT64);

    // operator ~
    // sign of output must be different than sign of input
    // if input is uint64, cast to int64 before the operation (we suppose its value is legal)
    cc = ~100ull / 10;              // ko

    cc = ~(int64_t)100ull / 10;     // ok

    aa = ~MAX_INT32;
    bb = ~(-MAX_INT32-1);   // critical value !!!
    cc = ~MAX_INT64;
    dd = ~(-MAX_INT64);

    // operator ^2
    // if input is int32, cast input to int64 if output requires 64 bits
    // if input is int64, cast output to uint64 if not representable by int64 -> NO !!
    aa = sing::pow2(MAX_INT32);                 // ko
    bb = sing::pow2(0xffffffffll) / 10;

    aa = sing::pow2((int64_t)MAX_INT32);         // ok
    bb = (uint64_t)sing::pow2(0xffffffffll) / 10;

    // operator ^
    // if the result and operands fits into int32, cast the inputs to int32 (those who are not yet !). 
    // if the result and operands fits into int64, cast the inputs to int64 (those who are not yet !). 
    // if the result fits into uint64, cast the inputs to int64 (those who are not yet !) and the output to uint64_t. -> NO !!
    // if the result and operands fits into uint64, cast the inputs to uint64 (those who are not yet !). 
    aa = sing::pow((int32_t)100ll, 2);         // ok
    bb = sing::pow((int64_t)100, (int64_t)5);
    cc = (uint64_t)sing::pow(0xffffffffll, (int64_t)2) / 10;

    cc = sing::pow(0xffffffffll, (int64_t)2) / 10;  // ko

    //
    // HERE ON TO FIX (See the warnings !!!)
    //

    // operator +, -, *, /, %
    // if any input is uint64:
        // if uint64 can't represent inputs and result but int64 can, cast all non-int64 inputs to int64
    // else if any input is int64:
        // if int64 can't represent inputs and result but uint64 can, cast any input to uint64
    // else if both inputs are int32 and output doesn't fit int32, cast any of the inputs to int64. 
    aa = VAL2EXP63 + (-1);
    bb = 100u + (-1);
    cc = (MAX_INT64 + MAX_INT64) / 10;
    dd = MAX_INT32 + MAX_INT32;

    aa = (int64_t)VAL2EXP63 + (-1);     // strange test (int64_t) can't represent the first arg.
    bb = (int64_t)100u + (-1);
    cc = ((uint64_t)MAX_INT64 + MAX_INT64) / 10;
    dd = (int64_t)MAX_INT32 + MAX_INT32;

//-------------

    aa = VAL2EXP63 - MAX_INT64 - 100;
    bb = 100u - (-1);
    dd = MAX_INT32 - (-MAX_INT32);

    aa = (int64_t)VAL2EXP63 - MAX_INT64 - 100;     // strange test (int64_t) can't represent the first arg.
    bb = (int64_t)100u - (-1);
    dd = (int64_t)MAX_INT32 - (-MAX_INT32);

//-----------

    bb = (MAX_INT64 * 2) / 10;
    cc = MAX_INT32 * MAX_INT32;

    bb = ((uint64_t)MAX_INT64 * 2) / 10;
    cc = (int64_t)MAX_INT32 * MAX_INT32;

//-------------

    aa = 100u / (-1);
    cc = MAX_INT32 / -1;

    aa = (int64_t)100u / (-1);
    cc = (int64_t)MAX_INT32 / -1;

//-------------

    aa = 100u % (-1);
    bb = 128 % 100;
    cc = 128 % -100;
    dd = -128 % 100;
    aa = -128 % -100;

    aa = (int64_t)100u % (-1);

//--------------

    
    // operators <<
    // if the left operand is an int32 and the result is not representable by int32, cast it to int64
    aa = MAX_INT32 << 2;
    bb = -MAX_INT32 << 2;

    aa = (int64_t)MAX_INT32 << 2;
    bb = (int64_t)-MAX_INT32 << 2;

    // operator >>, & are always ok
    aa = (VAL2EXP63 & VAL2EXP63) / 10;
    bb = (VAL2EXP63 & (-MAX_INT64-1)) / 10;
    cc = -1 & -1;

    // operator |, xor
    // if any of the operators is uint64 and the other is < 0, cast uint64 to int64
    aa = (VAL2EXP63 | VAL2EXP63) / 10;
    bb = (VAL2EXP63 | (-MAX_INT64 - 1)) / 10;
    cc = -1 | -1;
    
    bb = ((int64_t)VAL2EXP63 | (-MAX_INT64 - 1)) / 10;

    std::complex<float> cmp1 = 1.0f;
    std::complex<double> cmp2 = std::complex<float>(0, 1);

    cmp1 = cmp1 + sing::c_d2f(cmp2); //std::complex<double>(0, 5.6);
    float rpart = cmp1.real();
    //cmp_small = std::complex<float>(0, 1);

    // comparisons
    // if one of the twos is uint64 and the other is < 1 you need to use sing::ismore() or sing::isequal()
    bool comp = 5.6f > 100;
    comp = 5.6f > 100ll;
    comp = 100u > -1;
    comp = sing::ismore(100llu, (int64_t)-1);
    comp = sing::ismore(100u, -1);
    comp = MAX_INT64 > 100;
    
    char ch1 = 4;
    short ch2 = 10;

    cc = sing::pow(ch1, ch2);
    ch1 = 100;
    ch2 = sing::pow2((int32_t)ch1);
    ch1 = sing::pow2(ch1);

    uint32_t pwt = 0xffffffff;
    pwt = sing::pow(pwt, (uint32_t)2);

    cmp2 = std::pow(cmp2, 3.0);
    cmp2 = cmp2 + 10.0;
    cmp2 = std::pow(3.0, cmp2);
    rpart = std::pow(rpart, ch1);
    //rpart = std::powf(rpart, ch1);

    // casting with suffixes
    int32_t std = 100;
    int64_t big = 2147483648ll;
    uint64_t ubig = 9223372036854775808ull;
    double dbl = 1.0;

    // to uint64
    ubig = 100ull;
    ubig = 2147483648ull;
    ubig = 1ull;

    // to int64
    big = 100ll;
    big = 1ll;

    // to uint32
    ubig = 100u;                      // int32
    ubig = 2147483648u;               // int64
    ubig = 1u;                        // double

    // to int32
    std = 100;                       // int32
    std = 1;                         // double

    // to float
    dbl = 100.0f;
    dbl = 2147483648.0f;
    dbl = 9223372036854775808.0f;
    dbl = 1.0f;

    // to double
    dbl = 100.0;
    dbl = 2147483648.0;
    dbl = 9223372036854775808.0;

    // casting negatives with suffixes
    // note: unsigned are cast to signed before applying - !!
    std = -100;                        // int32
    big = -2147483648ll;               // int64
    dbl = -1.0;                              // double

    // to int64
    big = -100ll;
    big = -1ll;
                               // to int32
    std = -100;                       // int32
    std = -1;                        // double

                              // to float
    dbl = -100.0f;
    dbl = -2147483648.0f;
    dbl = -1.0f;

    // to double
    dbl = -100.0;
    dbl = -2147483648.0;

    uint16_t uch1 = 0x8000;
    bool issigned = (uch1 << 16) < 0;

    double pt0 = 0x20000000000000;
    int64_t pt0check = (int64_t)pt0;
    pt0 = 0x1fffffffffffff;
    pt0check = (int64_t)pt0;
    pt0 = 0x20000000000001;
    pt0check = (int64_t)pt0;

    float pt1 = 0x1000000;
    pt0check = (int64_t)pt1;
    pt1 = 0xffffff;
    pt0check = (int64_t)pt1;
    pt1 = 0x1000001;
    pt0check = (int64_t)pt1;

    int pippo = (int32_t)-0x80000000ll >> 3;
    float ttt = atof("-5");
}