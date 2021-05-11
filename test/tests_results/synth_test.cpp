#include "synth_test.h"

typedef int32_t myint;

static const int32_t vvv = 5;

static void receiver(const sing::array<sing::array<std::string, 4>, 4> &vin, sing::array<sing::array<std::string, 4>, 4> *vout);
static int32_t add_int(int32_t val0, int32_t val1 = 5);
static void add_int3(int32_t val0, int32_t val1, int32_t *valout);
static void forward(myint val);
static void expressions();
static void funwithdefs(int32_t a0 = 100, const char *a1 = "ciccio" "franco", bool a3 = vvv > 2, std::shared_ptr<int32_t> a4 = nullptr);
static void minmaxswap();
static void string_tests(const char *arg);
static void string_copy(const char *arg, std::string *aout);

static std::shared_ptr<const std::string> gp;

static const std::vector<myint> mivv;

static void receiver(const sing::array<sing::array<std::string, 4>, 4> &vin, sing::array<sing::array<std::string, 4>, 4> *vout)
{
}

static int32_t add_int(int32_t val0, int32_t val1)
{
    return (val0 + val1);
}

static void add_int3(int32_t val0, int32_t val1, int32_t *valout)
{
    *valout = val0 + val1;
}

int32_t synth_test()
{
    // consts declarations (heap and not-heap)
    std::shared_ptr<const std::string> cs = std::make_shared<std::string>("test");
    const int32_t ci = 123;
    gp = cs;

    // 0 initer of defined type
    const myint miv = 0;

    // array of array passing (the first index is wildcharted)
    sing::array<sing::array<std::string, 4>, 4> arr;
    receiver(arr, &arr);

    // default parms
    const int32_t res1 = add_int(1, 2);
    const int32_t res2 = add_int(4);

    // init with automatic or computed length.
    sing::array<int32_t, 4> arr2 = {1, 2, 3, 4};
    sing::array<int32_t, 5 * 3> arr3 = {0};

    // inc and dec
    ++arr2[0];
    --arr2[1];
    while (arr2[2] > 0) {
        --arr2[2];
        if ((arr2[2] & 1) != 0) {
            continue;
        }
        ++arr2[0];
    }

    // if else and block
    {
        int32_t res = 0;
        for(int32_t idx = 0; idx < 10; ++idx) {
            if (idx == 0) {
                res += 1;
            } else if (idx == 1) {
                res *= 3;
            } else {
                res += 2;
            }
        }
    }

    // for/foreach iterations
    // with/without count
    // new/ not new iterator
    // literal/not literal step
    // positive/negative step
    // requiring conversions or not.
    int32_t it = 0;
    int64_t count = 0;
    for(int32_t idx = 1, idx__step = arr2[0]; idx__step > 0 ? (idx < 10) : (idx > 10); idx += idx__step, ++count) {
        ++it;
    }

    it = 0;
    count = 0;
    for(int32_t idx = 10; idx > 1; --idx, ++count) {
        ++it;
    }
    count = 0;
    for(int32_t idx = 10; idx > 1; --idx, ++count) {
        ++it;
    }
    count = 0;
    for(int32_t idx = 10; idx > 1; idx += (int32_t)(-20000000000LL / 10000000000LL), ++count) {
        ++it;
    }

    it = 0;
    count = -1;
    for(auto &iteratedint : arr2) {
        ++count;
        ++it;
    }

    expressions();

    // pointer
    std::shared_ptr<sing::array<int32_t, 10>> onheap = std::make_shared<sing::array<int32_t, 10>>();
    *onheap = {0};
    std::shared_ptr<sing::array<int32_t, 10>> ohp = onheap;
    (*ohp)[3] = 100;
    (*onheap)[3] += 1;

    // sizeof and dimof
    int32_t v_int32 = 0;
    v_int32 = sizeof(int8_t (*)(int32_t a1));
    v_int32 = sizeof((*onheap)[2]);

    // simplifying &*
    std::shared_ptr<int32_t> intonheap = std::make_shared<int32_t>(10);
    std::shared_ptr<int32_t> iohp = intonheap;
    add_int3(3, 2, &*intonheap);

    forward(5);

    // using a const int to init an array size
    const int32_t arraydim = -100 / 2;
    sing::array<float, -arraydim * 2> fvec = {0};
    fvec[99] = (float)0;

    // automatic types
    int32_t autoint = 100;
    float autofloat = 10.0f;
    std::complex<float> autocomplex = std::complex<float>(0.0f, 10.0f);
    bool autobool = autoint < 0;
    std::string autostring = "goophy";

    string_tests("bambam");

    // return needs conversion
    return ((int32_t)10.0f);
}

static void forward(myint val)
{
}

static void expressions()
{
    // all the types
    int8_t v_int8 = (int8_t)1;
    uint8_t v_uint8 = (uint8_t)1;
    int32_t v_rune = 2;
    int32_t v_size = 3;
    int32_t v_int32 = 4;
    uint32_t v_uint32 = (uint32_t)5;
    int64_t v_int64 = 4;
    uint64_t v_uint64 = (uint64_t)5;
    float f0 = 1.0f;
    double d0 = 2.0f;
    std::complex<float> c0 = 1.0f + std::complex<float>(0.0f, 1.0f);
    std::complex<double> c1 = std::complex<double>(1.0, 1.0);

    // sum of srings
    std::string s0 = "aaa";
    std::string s1 = "bbb";
    s0 = s0 + s1;
    c0 = 1.0f + std::complex<float>(0.0f, 1.0f);
    c1 = std::complex<double>(1.0, 1.0);
    s0 = sing::s_format("%s%s%f%d%s%d%u%u%s%s", s0.c_str(), s1.c_str(), f0, v_int32, "false", v_int8, v_uint32, v_uint8, sing::to_string(c0).c_str(),
        sing::to_string(c1).c_str());
    s0 = sing::s_format("%s%s%s%s", s0.c_str(), "f", "alse", s1.c_str());
    const bool b_for_print = true;
    const bool b_for_false = false;
    s0 = sing::s_format("%s%s%s", b_for_print ? "true" : "false", "true", b_for_false ? "true" : "false");

    // power + cases which require conversion
    v_int32 = sing::pow2((int32_t)v_int8);                  // **2 integer promotion
    v_int32 = sing::pow(v_int32, v_int8);                   // **, <<, >> on ints, don't require the operands to be of the same type 
    f0 = std::pow(f0, f0);              // same types: no conversion needed
    c0 = std::pow(c0, f0);              // floats is like complex without a component !
    c0 = std::pow(f0, c0);
    c1 = std::pow(c1, d0);
    c1 = std::pow(d0, c1);
    f0 = std::pow(f0, 3.0f);
    f0 = std::pow(3.0f, f0);
    v_int32 = sing::pow((int32_t)v_int8, v_uint32);         // integer promotion

    // xor update
    v_int32 ^= 2;
    v_int32 ^= 3;
    //f0 ^= 2.0;

    // math operators
    f0 = f0 + (float)(34 >> 1);         // >> needs brackets and conversion
    f0 = (float)(34 >> 1) + f0;
    v_int32 = v_int32 ^ 3;

    // relationals
    bool b0 = v_int8 > 3;
    bool b1 = sing::ismore(v_uint32, v_int32);
    b0 = sing::ismore_eq(v_uint32, v_int32);
    b0 = sing::isless(v_uint32, v_int32);
    b0 = sing::isless_eq(v_uint32, v_int32);
    b0 = sing::iseq(v_uint32, v_int32);
    b0 = !sing::iseq(v_uint32, v_int32);
    b0 = sing::isless(v_uint32, v_int32);
    b0 = sing::isless_eq(v_uint32, v_int32);
    b0 = sing::ismore(v_uint32, v_int32);
    b0 = sing::ismore_eq(v_uint32, v_int32);
    b0 = v_int8 > v_uint8;
    b0 = sing::iseq(v_uint32, v_int32);
    b0 = !sing::iseq(v_uint32, v_int32);
    b0 = sing::c_f2d(c0) == c1;
    b0 = c0 == f0;
    b0 = s0 > "big";
    b0 = "big" > s0;
    b0 = b0 != b1;
    b0 = c1 == (double)10;
    b0 = c0 == std::complex<float>(0.0f, 3.0f);
    b0 = c0 == (float)10;
    b0 = d0 > 10;
    b0 = f0 > (float)10;

    // logicals
    if (v_int32 > 10 && v_int8 > 0 || b0) {
        ++v_int32;
    }

    // casts
    d0 = (double)c0.real() + (double)c0.real();
    s0 = "1234";
    v_uint32 = (uint32_t)(sing::string2uint(s0));
    s0 = "-123";
    v_int32 = (int32_t)(sing::string2int(s0));
    s0 = "100e-3";
    f0 = (float)(sing::string2double(s0));
    c0 = sing::c_d2f(c1);
    c1 = sing::c_f2d(c0);
    c1 = (std::complex<double>)v_int32;
    c1 = (std::complex<double>)v_int8;
    c0 = (std::complex<float>)f0;
    c0 = (std::complex<float>)v_int8;
    c0 = (std::complex<float>)(float)v_int32;
    s0 = std::to_string(v_int32);
    s0 = std::to_string(f0);
    s0 = sing::to_string(b0);
    s0 = "100 + 3i";
    c0 = sing::string2complex64(s0);
    c1 = sing::string2complex128(s0);
    v_int32 = (int8_t)0x1ff;
    v_int32 = (int16_t)0x1ffff;
    v_uint64 = 200000000000000000LLU;

    // unary
    v_int32 += sizeof(v_int32);
    v_int64 += (int64_t)~2;
    v_uint64 += (uint64_t)-3LL;
    v_uint64 = (uint32_t)~2;

    // literal to variable automatic downcasts
    c1 = (double)100;
    c0 = 1.0f + std::complex<float>(0.0f, 1.0f);
    c0 = (float)100;
    d0 = (1.0f + std::complex<float>(0.0f, 1.0f) - std::complex<float>(0.0f, 1.0f)).real();
    f0 = 1.0f;
    v_int32 = (int32_t)1.0f;
    c1 = (double)1.0f;

    // fake casts
    v_int32 = 1000;
    v_int32 = -100000;
    v_int64 = 1000LL;
    v_int64 = -0xffffLL;
    v_uint32 = 1000U;
    v_uint32 = 100000U;
    v_uint64 = 1000LLU;
    v_uint64 = 0xffffLLU;
    f0 = 1000.0f;
    f0 = -1000e2f;
    d0 = 1000.0;
    d0 = -65535.0;
    c0 = std::complex<float>(1000.0f, 1000.0f);
    c0 = std::complex<float>(-1000e2f, -1000e2f);
    c1 = std::complex<double>(1000.0, 1000.0);
    c1 = std::complex<double>(-65535.0, 0.0);
    c0 = std::complex<float>(100.0f, 0.0f);

    // extreme values for signed ints
    v_int32 = 0x7fffffff;
    v_int32 = (int32_t)-0x80000000LL;
    v_int64 = 0x7fffffffffffffffLL;
    v_int64 = -0x8000000000000000LL;

    // compile time constants checks
    v_int32 = sing::pow(5, 3);
    v_int32 = 5 + -3;
    v_int32 = -5 + -3;
    v_int32 = -5 + 3;
    v_int32 = 5 + 3;
    v_int32 = 5 - -3;
    v_int32 = -5 - -3;
    v_int32 = -5 - 3;
    v_int32 = 5 - 3;
    v_int32 = 5 * 12;
    v_int32 = 183 / 11;
    v_int32 = 183 % 11;
    v_int32 = 12000 >> 3;
    v_int32 = 12000 << 3;
    v_int32 = -12000 >> 3;
    v_int32 = -12000 << 3;
    v_int32 = (int32_t)((uint32_t)-12000 >> 3);
    v_int32 = 5 | 100;
    v_int32 = -12 & 100;
    v_int32 = -12 ^ 100;
    f0 = (float)(sing::pow2(3.0));
    f0 = (float)(3.0 + 2.0);
    f0 = (float)(3.0 - 2.0);
    f0 = (float)(3.0 * 2.0);
    f0 = (float)(3.0 / 2.0);
    f0 = sing::pow2(3.0f);
    f0 = 3.0f + 2.0f;
    f0 = 3.0f - 2.0f;
    f0 = 3.0f * 2.0f;
    f0 = 3.0f / 2.0f;
    c1 = std::pow(std::complex<double>(3.0, 0.0), std::complex<double>(2.0, 1.0));
    c1 = std::complex<double>(3.0, 0.0) + std::complex<double>(2.0, 1.0);
    c1 = std::complex<double>(3.0, 0.0) - std::complex<double>(2.0, 1.0);
    c1 = std::complex<double>(3.0, 0.0) * std::complex<double>(2.0, 1.0);
    c1 = std::complex<double>(3.0, 0.0) / std::complex<double>(2.0, 1.0);
    c1 = sing::c_f2d(std::pow(3.0f, 2.0f + std::complex<float>(0.0f, 1.0f)));
    c1 = sing::c_f2d(3.0f + (2.0f + std::complex<float>(0.0f, 1.0f)));
    c1 = sing::c_f2d(3.0f - (2.0f + std::complex<float>(0.0f, 1.0f)));
    c1 = sing::c_f2d(3.0f * (2.0f + std::complex<float>(0.0f, 1.0f)));
    c1 = sing::c_f2d(3.0f / (2.0f + std::complex<float>(0.0f, 1.0f)));
}

// functions with defaults
static void funwithdefs(int32_t a0, const char *a1, bool a3, std::shared_ptr<int32_t> a4)
{
    const std::string aaa = sing::s_format("%s%s", "ciccio", a1);
    const std::string bbb = sing::s_format("%s%s", a1, "ciccio");
}

static void minmaxswap()
{
    int32_t pippo = std::max(3, 6);
    int32_t pluto = std::min(3, 6);
    std::swap(pippo, pluto);
}

static void string_tests(const char *arg)
{
    std::string acc = "a";
    std::string s0 = "b";
    std::string s1 = "c";
    bool comp = false;

    // append
    acc += arg;
    acc += "literal";
    acc += s0;

    // add strings
    acc = s0 + s1;

    // add string + const string 
    acc = s0 + arg;
    acc = s0 + "literal";
    acc = arg + s0;
    acc = "literal" + s0;

    // add const strings
    acc = sing::s_format("%s%s", arg, "literal");
    acc = sing::s_format("%s%s", arg, arg);
    acc = sing::s_format("%s%s", "literal", arg);
    acc = "lit" "eral";

    // add three or more
    acc = sing::s_format("%s%s%s", arg, s0.c_str(), "lit");

    // compare 
    comp = s0 < s1;
    comp = s0 < "lit";
    comp = "lit" < s0;
    comp = s0 < arg;
    comp = arg < s0;
    comp = ::strcmp(arg, "lit") < 0;
    comp = ::strcmp("lit", "literal") < 0;

    // parm passing
    string_copy(s0.c_str(), &acc);
    string_copy(arg, &acc);
    string_copy("k", &acc);
}

static void string_copy(const char *arg, std::string *aout)
{
    *aout = arg;
}
