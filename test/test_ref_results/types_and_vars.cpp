#include "types_and_vars.h"

typedef int32_t (*fref)(const int32_t arg0, const int32_t arg1, int32_t *arg2);

static int32_t intsum(const int32_t arg0, const int32_t arg1, int32_t *arg2);
static bool boolsum(const bool arg0, const bool arg1, bool *arg2);
static std::string stringsum(const char *arg0, const char *arg1, std::string *arg2);
static fref frefshift(const fref arg0, fref *arg1, fref *arg2);
static void arrayswap(const sing::vect<int32_t> &arg0, sing::vect<int32_t> &arg1);
static void dynaswap(const sing::vect<int32_t> &arg0, sing::vect<int32_t> &arg1, const int32_t count);
static void stringswap(const sing::vect<std::string> &arg0, sing::vect<std::string> &arg1, const int32_t count);

// gloabl declarations, no initer
static int32_t gv_int32;
static bool gv_bool;
static std::string gv_string;
static fref gv_fref;
static sing::ptr<int32_t> gv_ptr;
static sing::spvect<int32_t, 5> gv_array;
static sing::dpvect<int32_t> gv_dyna;
static sing::dvect<std::string> gv_stringarr;

// gloabl declarations, inited
static int32_t gvi_int32 = 100;
static bool gvi_bool = true;
static std::string gvi_string = "pippi";
static fref gvi_fref = intsum;
//var gvi_ptr *i32;            // allowed ?
static sing::spvect<int32_t, 5> gvi_array = {1, 2, 3, 4, 5};
static sing::dpvect<int32_t> gvi_dyna = {6, 7, 8, 9, 10};
static sing::dvect<std::string> gvi_stringarr = {"minni", "clara", "ciccio"};

static int32_t intsum(const int32_t arg0, const int32_t arg1, int32_t *arg2)
{
    *arg2 = arg0 + arg1;
    return (*arg2);
}

// as parameters
static bool boolsum(const bool arg0, const bool arg1, bool *arg2)
{
    *arg2 = arg0 || arg1;
    return (*arg2);
}

static std::string stringsum(const char *arg0, const char *arg1, std::string *arg2)
{
    *arg2 = sing::s_format("%s%s", arg0, arg1);
    return (*arg2);
}

static fref frefshift(const fref arg0, fref *arg1, fref *arg2)
{
    fref tmp = *arg2;

    *arg2 = *arg1;
    *arg1 = arg0;
    return (tmp);
}

static void arrayswap(const sing::vect<int32_t> &arg0, sing::vect<int32_t> &arg1)
{
    arg1[0] = arg0[4];
    arg1[1] = arg0[3];
    arg1[2] = arg0[2];
    arg1[3] = arg0[1];
    arg1[4] = arg0[0];
}

static void dynaswap(const sing::vect<int32_t> &arg0, sing::vect<int32_t> &arg1, const int32_t count)
{
    const int32_t top = count - 1;

    uint64_t idx = 0;
    for(int32_t *dst = arg1.begin(); dst < arg1.end(); ++dst, ++idx) {
        *dst = arg0[top - (int32_t)idx];
    }
}

static void stringswap(const sing::vect<std::string> &arg0, sing::vect<std::string> &arg1, const int32_t count)
{
    const int32_t top = count - 1;

    uint64_t idx = 0;
    for(std::string *dst = arg1.begin(); dst < arg1.end(); ++dst, ++idx) {
        *dst = arg0[top - (int32_t)idx];
    }
}

void test_types_and_vars()
{
    // gloabl declarations, no initer
    int32_t lv_int32 = 0;
    bool lv_bool = false;
    std::string lv_string;
    int32_t (*lv_fref)(const int32_t arg0, const int32_t arg1, int32_t *arg2) = nullptr;
    sing::ptr<int32_t> lv_ptr;
    sing::spvect<int32_t, 5> lv_array;
    sing::dpvect<int32_t> lv_dyna;
    sing::dvect<std::string> lv_stringarr;

    // local declarations, inited
    int32_t lvi_int32 = 100;
    bool lvi_bool = true;
    std::string lvi_string = "pippi";
    int32_t (*lvi_fref)(const int32_t arg0, const int32_t arg1, int32_t *arg2) = intsum;
    //var lvi_ptr *i32;            // allowed ?
    sing::spvect<int32_t, 5> lvi_array = {1, 2, 3, 4, 5};
    sing::dpvect<int32_t> lvi_dyna = {6, 7, 8, 9, 10};
    sing::dvect<std::string> lvi_stringarr = {"minni", "clara", "ciccio"};

    // heap declarations, no initer
    sing::ptr<int32_t> hv_int32(new sing::wrapper<int32_t>(0));
    sing::ptr<bool> hv_bool(new sing::wrapper<bool>(false));
    sing::ptr<std::string> hv_string(new sing::wrapper<std::string>);
    sing::ptr<int32_t (*)(const int32_t arg0, const int32_t arg1, int32_t *arg2)> hv_fref(new sing::wrapper<int32_t (*)(const int32_t arg0,
        const int32_t arg1, int32_t *arg2)>(nullptr));
    sing::ptr<sing::ptr<int32_t>> hv_ptr(new sing::wrapper<sing::ptr<int32_t>>);
    sing::ptr<sing::spvect<int32_t, 5>> hv_array(new sing::wrapper<sing::spvect<int32_t, 5>>);
    sing::ptr<sing::dpvect<int32_t>> hv_dyna(new sing::wrapper<sing::dpvect<int32_t>>);
    sing::ptr<sing::dvect<std::string>> hv_stringarr(new sing::wrapper<sing::dvect<std::string>>);

    // heap declarations, inited
    sing::ptr<int32_t> hvi_int32(new sing::wrapper<int32_t>(100));
    sing::ptr<bool> hvi_bool(new sing::wrapper<bool>(true));
    sing::ptr<std::string> hvi_string(new sing::wrapper<std::string>("pippi"));
    sing::ptr<fref> hvi_fref(new sing::wrapper<fref>(intsum));
    //var lvi_ptr *i32;            // allowed ?
    sing::ptr<sing::spvect<int32_t, 5>> hvi_array(new sing::wrapper<sing::spvect<int32_t, 5>>);
    *hvi_array = {1, 2, 3, 4, 5};
    sing::ptr<sing::dpvect<int32_t>> hvi_dyna(new sing::wrapper<sing::dpvect<int32_t>>);
    *hvi_dyna = {6, 7, 8, 9, 10};
    sing::ptr<sing::dvect<std::string>> hvi_stringarr(new sing::wrapper<sing::dvect<std::string>>);
    *hvi_stringarr = {"minni", "clara", "ciccio"};

    // pointers to heap-allocated vars
    sing::ptr<int32_t> pv_int32 = hv_int32;
    sing::ptr<bool> pv_bool = hv_bool;
    sing::ptr<std::string> pv_string = hv_string;
    sing::ptr<int32_t (*)(const int32_t arg0, const int32_t arg1, int32_t *arg2)> pv_fref = hv_fref;
    sing::ptr<sing::ptr<int32_t>> pv_ptr = hv_ptr;
    sing::ptr<sing::spvect<int32_t, 5>> pv_array = hv_array;
    sing::ptr<sing::dpvect<int32_t>> pv_dyna = hv_dyna;
    sing::ptr<sing::dvect<std::string>> pv_stringarr = hv_stringarr;

    pv_int32 = hvi_int32;
    pv_bool = hvi_bool;
    pv_string = hvi_string;
    pv_fref = hvi_fref;
    //pv_ptr  = hvi_ptr;
    pv_array = hvi_array;
    pv_dyna = hvi_dyna;
    pv_stringarr = hvi_stringarr;

    // iterators and iterators' references: several addressings
    for(int32_t *intiterator = lvi_array.begin(); intiterator < lvi_array.end(); ++intiterator) {
        ++(*intiterator);
    }
    for(int32_t *intiterator = (*hvi_array).begin(); intiterator < (*hvi_array).end(); ++intiterator) {
        ++(*intiterator);
    }
    for(int32_t *intiterator = (*pv_array).begin(); intiterator < (*pv_array).end(); ++intiterator) {
        ++(*intiterator);
    }

    // iterators on several iterated types
    sing::spvect<bool, 3> boolvec = {false, false, true};

    for(bool *booliterator = boolvec.begin(); booliterator < boolvec.end(); ++booliterator) {
        *booliterator = !*booliterator;
    }

    for(std::string *stringiterator = lvi_stringarr.begin(); stringiterator < lvi_stringarr.end(); ++stringiterator) {
        *stringiterator += "!";
    }

    sing::svect<sing::ptr<std::string>, 2> ptrvec = {hv_string, hvi_string};

    for(sing::ptr<std::string> *ptriterator = ptrvec.begin(); ptriterator < ptrvec.end(); ++ptriterator) {
        **ptriterator += "-";
    }

    sing::svect<sing::svect<std::string, 3>, 3> arrvec = {{"a", "b", "c"}, {"d", "e"}, {"f", "g"}};

    for(sing::svect<std::string, 3> *arrayiterator = arrvec.begin(); arrayiterator < arrvec.end(); ++arrayiterator) {
        for(std::string *stringiterator = (*arrayiterator).begin(); stringiterator < (*arrayiterator).end(); ++stringiterator) {
            *stringiterator += "_ok";
        }
    }

    sing::dvect<sing::dvect<std::string>> arrdyna = {{"a", "b", "c"}, {"d", "e"}, {"f", "g"}};

    for(sing::dvect<std::string> *arrayiterator = arrdyna.begin(); arrayiterator < arrdyna.end(); ++arrayiterator) {
        for(std::string *stringiterator = (*arrayiterator).begin(); stringiterator < (*arrayiterator).end(); ++stringiterator) {
            *stringiterator += "_ok";
        }
    }

    // assignments
    lv_int32 = gvi_int32;
    lv_bool = gvi_bool;
    lv_string = gvi_string;
    lv_fref = gvi_fref;
    lv_ptr = pv_int32;
    lv_array = gvi_array;
    lv_dyna = gvi_dyna;
    lv_stringarr = gvi_stringarr;

    *hv_int32 = gvi_int32;
    *hv_bool = gvi_bool;
    *hv_string = gvi_string;
    *hv_fref = gvi_fref;
    *hv_ptr = pv_int32;
    *hv_array = gvi_array;
    *hv_dyna = gvi_dyna;
    *hv_stringarr = gvi_stringarr;

    // passing arguments
    lv_int32 = gvi_fref(lv_int32, lvi_int32, &lv_int32);
    lv_int32 = intsum(lv_int32, lvi_int32, &lv_int32);
    lv_bool = boolsum(lv_bool, lvi_bool, &lv_bool);
    lv_string = stringsum(lv_string.c_str(), lvi_string.c_str(), &lv_string);
    gv_fref = frefshift(*hvi_fref, &lv_fref, &gvi_fref);
    arrayswap(lvi_array, lv_array);
    dynaswap(lvi_dyna, lv_dyna, 5);
    stringswap(*pv_stringarr, lv_stringarr, 3);
}
