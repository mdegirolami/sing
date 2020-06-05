#include "formatter_test.h"

static void manyargs(const std::complex<float> veryveryveryveryveryverylongarg1, const std::complex<float> veryveryveryveryveryverylongarg2,
    const std::complex<float> veryveryveryveryveryverylongarg3, const std::complex<float> veryveryveryveryveryverylongarg4);
static void fn1(const int32_t a, const int32_t b, int32_t *c);
static void fn2(const int32_t a, const int32_t b, int32_t *c);

static const int32_t veryveryveryveryveryverylong1 = 100;
static const int32_t veryveryveryveryveryverylong2 = 100;
static const int32_t veryveryveryveryveryverylong3 = 100;
static const int32_t veryveryveryveryveryverylong4 = 100;

// attached to next
static const sing::svect<sing::spvect<int32_t, 3>, 6> table = {{1, 2, 3}, {1, 2, 3}, {1, 2, 3},     // hand formatted !!
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}                                                                 // following
};
// attached to prev

static const sing::svect<sing::svect<sing::svect<std::string, 3>, 3>, 3> table2 = {{{"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}}, {{"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}}, {{"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}}};

static const sing::svect<sing::spvect<int32_t, 3>, 6> table3 = {                // just one remark
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}, 
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}
};

static const std::string stest = "kjdhfkhfdkhskdfhkshghkhl" // rem1
    "hfdslkjflkjasdlkfjlksdjflkj";                          // rem2

static void manyargs(const std::complex<float> veryveryveryveryveryverylongarg1, const std::complex<float> veryveryveryveryveryverylongarg2,
    const std::complex<float> veryveryveryveryveryverylongarg3, const std::complex<float> veryveryveryveryveryverylongarg4)
{
}

// next

// next

static void fn1(const int32_t a, const int32_t b, int32_t *c)                   // rem1
{                   // rem2
                // rem3
    // of statement
    *c = a + b;     // post

    *c = 100 + 123 * sing::pow2(12) + 12345 / (veryveryveryveryveryverylong1 / veryveryveryveryveryverylong2) -         // rem
        (veryveryveryveryveryverylong3 & veryveryveryveryveryverylong4);
    manyargs(12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f),                // xyz
        12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f));
    manyargs(12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f),                // abc 
        12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f));                    // abc
                                    // abc
                                     // xyz

    // note: the next line has blanks but must be handled as empty

    for(int32_t it = 0; it < 10; ++it) {                    // onfor
        ++(*c);
        break;      // inner
    }
}

static void fn2(const int32_t a, const int32_t b, int32_t *c)                   // rem1
{                   // rem2
}                   // rem3
