#include "formatter_test.h"

static const int32_t veryveryveryveryveryverylong1 = 100;
static const int32_t veryveryveryveryveryverylong2 = 100;
static const int32_t veryveryveryveryveryverylong3 = 100;
static const int32_t veryveryveryveryveryverylong4 = 100;

// before class
class tc final {    // on class
public:
    tc();

    // on ff1
    void ff1();

    int32_t pub_;                       // on pub

private:
    // on ff0
    void ff0();

    int32_t priv1_;
    int32_t priv2_;                     // on priv2
};

class xxx {         // on if
public:
    virtual ~xxx() {}
    virtual void *get__id() const = 0;
    virtual void dostuff() const = 0;                       // on fun
    virtual void dootherstuff() const = 0;
};

static void manyargs(const std::complex<float> veryveryveryveryveryverylongarg1, const std::complex<float> veryveryveryveryveryverylongarg2,
    const std::complex<float> veryveryveryveryveryverylongarg3, const std::complex<float> veryveryveryveryveryverylongarg4);
static void fn1(const int32_t a, const int32_t b, int32_t *c);
static void fn2(const int32_t a, const int32_t b, int32_t *c);

// attached to next
static const sing::array<sing::array<int32_t, 3>, 6> table = {{1, 2, 3}, {1, 2, 3}, {1, 2, 3},      // hand formatted !!
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}
};
// attached to table4

static const sing::array<sing::array<int32_t, 3>, 6> table4 = {
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}, 
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}
};

static const sing::array<sing::array<sing::array<std::string, 3>, 3>, 3> table2 = {{{"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}}, {{"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}}, {{"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}, {"akjhdfk", "askjfhsad", "hgfksahgjh"}}};

static const sing::array<sing::array<int32_t, 3>, 6> table3 = {
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}, 
    {1, 2, 3}, {1, 2, 3}, {1, 2, 3}
};

static const std::string stest = "kjdhfkhfdkhskdfhkshghkhl"                     // rem1
    "hfdslkjflkjasdlkfjlksdjflkj";

static void manyargs(const std::complex<float> veryveryveryveryveryverylongarg1, const std::complex<float> veryveryveryveryveryverylongarg2,
    const std::complex<float> veryveryveryveryveryverylongarg3, const std::complex<float> veryveryveryveryveryverylongarg4)
{
}

static void fn1(const int32_t a, const int32_t b, int32_t *c)                   // rem1
{
    // of statement
    *c = a + b;     // post

    *c = 100 + 123 * sing::pow2(12) + 12345 / (veryveryveryveryveryverylong1 / veryveryveryveryveryverylong2) -         // rem
        (veryveryveryveryveryverylong3 & veryveryveryveryveryverylong4);
    manyargs(12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f),                // xyz
        12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f));
    manyargs(12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f),                //xyz
        12.6526546f + std::complex<float>(0.0f, 12.925985478f), 12.6526546f + std::complex<float>(0.0f, 12.925985478f));

    // note: the next line has blanks but must be handled as empty

    for(int32_t it = 0; it < 10; ++it) {                    // onfor
        ++(*c);
        break;      // inner
    }
}

static void fn2(const int32_t a, const int32_t b, int32_t *c)                   // rem0
{
}

tc::tc()
{
    priv1_ = 0;
    priv2_ = 0;
    pub_ = 0;
}

void tc::ff0()
{
    priv1_ = 0;
}

void tc::ff1()
{
    switch (priv2_) {                   // on switch
    case 0: 

        ++pub_;     // on statement 0
        break;
    case 1: 
    case 2: 
        {           // on case2
            ff0();
        }
        break;
    }
}
