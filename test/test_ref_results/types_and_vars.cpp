#include "types_and_vars.h"

typedef int32_t (*fref)(int32_t arg0, int32_t arg1, int32_t *arg2);

static int32_t intsum(int32_t arg0, int32_t arg1, int32_t *arg2);
static bool boolsum(bool arg0, bool arg1, bool *arg2);
static std::string stringsum(const char *arg0, const char *arg1, std::string *arg2);
static fref frefshift(fref arg0, fref *arg1, fref *arg2);
static void arrayswap(const sing::array<int32_t, 5> &arg0, sing::array<int32_t, 5> *arg1);
static void dynaswap(const std::vector<int32_t> &arg0, std::vector<int32_t> *arg1, int32_t count);
static void stringswap(const std::vector<std::string> &arg0, std::vector<std::string> *arg1, int32_t count);

// gloabl declarations, no initer
static int32_t gv_int32;
static bool gv_bool;
static std::string gv_string;
static fref gv_fref;
static std::shared_ptr<int32_t> gv_ptr;
static sing::array<int32_t, 5> gv_array;
static std::vector<int32_t> gv_dyna;
static std::vector<std::string> gv_stringarr;

// gloabl declarations, inited
static int32_t gvi_int32 = 100;
static bool gvi_bool = true;
static std::string gvi_string = "pippi";
static fref gvi_fref = intsum;
//var gvi_ptr *i32;            // allowed ?
static sing::array<int32_t, 5> gvi_array = {1, 2, 3, 4, 5};
static std::vector<int32_t> gvi_dyna = {6, 7, 8, 9, 10};
static std::vector<std::string> gvi_stringarr = {"minni", "clara", "ciccio"};

static int32_t intsum(int32_t arg0, int32_t arg1, int32_t *arg2)
{
    *arg2 = arg0 + arg1;
    return (*arg2);
}

// as parameters
static bool boolsum(bool arg0, bool arg1, bool *arg2)
{
    *arg2 = arg0 || arg1;
    return (*arg2);
}

static std::string stringsum(const char *arg0, const char *arg1, std::string *arg2)
{
    *arg2 = sing::s_format("%s%s", arg0, arg1);
    return (*arg2);
}

static fref frefshift(fref arg0, fref *arg1, fref *arg2)
{
    fref tmp = *arg2;
    *arg2 = *arg1;
    *arg1 = arg0;
    return (tmp);
}

static void arrayswap(const sing::array<int32_t, 5> &arg0, sing::array<int32_t, 5> *arg1)
{
    (*arg1)[0] = arg0[4];
    (*arg1)[1] = arg0[3];
    (*arg1)[2] = arg0[2];
    (*arg1)[3] = arg0[1];
    (*arg1)[4] = arg0[0];
}

static void dynaswap(const std::vector<int32_t> &arg0, std::vector<int32_t> *arg1, int32_t count)
{
    int32_t src = count - 1;
    for(auto &dst : *arg1) {
        dst = arg0[src];
        --src;
    }
}

static void stringswap(const std::vector<std::string> &arg0, std::vector<std::string> *arg1, int32_t count)
{
    int32_t src = count - 1;
    for(auto &dst : *arg1) {
        dst = arg0[src];
        --src;
    }
}

void test_types_and_vars()
{
    // gloabl declarations, no initer
    int32_t lv_int32 = 0;
    bool lv_bool = false;
    std::string lv_string;
    int32_t (*lv_fref)(int32_t arg0, int32_t arg1, int32_t *arg2) = nullptr;
    std::shared_ptr<int32_t> lv_ptr;
    sing::array<int32_t, 5> lv_array = {0};
    std::vector<int32_t> lv_dyna;
    std::vector<std::string> lv_stringarr;

    // local declarations, inited
    int32_t lvi_int32 = 100;
    bool lvi_bool = true;
    std::string lvi_string = "pippi";
    int32_t (*lvi_fref)(int32_t arg0, int32_t arg1, int32_t *arg2) = intsum;
    //var lvi_ptr *i32;            // allowed ?
    sing::array<int32_t, 5> lvi_array = {1, 2, 3, 4, 5};
    std::vector<int32_t> lvi_dyna = {6, 7, 8, 9, 10};
    std::vector<std::string> lvi_stringarr = {"minni", "clara", "ciccio"};

    // heap declarations, no initer
    std::shared_ptr<int32_t> hv_int32 = std::make_shared<int32_t>(0);
    std::shared_ptr<bool> hv_bool = std::make_shared<bool>(false);
    std::shared_ptr<std::string> hv_string = std::make_shared<std::string>();
    std::shared_ptr<int32_t (*)(int32_t arg0, int32_t arg1, int32_t *arg2)> hv_fref = std::make_shared<int32_t (*)(int32_t arg0, int32_t arg1,
        int32_t *arg2)>(nullptr);
    std::shared_ptr<std::shared_ptr<int32_t>> hv_ptr = std::make_shared<std::shared_ptr<int32_t>>();
    std::shared_ptr<sing::array<int32_t, 5>> hv_array = std::make_shared<sing::array<int32_t, 5>>();
    *hv_array = {0};
    std::shared_ptr<std::vector<int32_t>> hv_dyna = std::make_shared<std::vector<int32_t>>();
    std::shared_ptr<std::vector<std::string>> hv_stringarr = std::make_shared<std::vector<std::string>>();

    // heap declarations, inited
    std::shared_ptr<int32_t> hvi_int32 = std::make_shared<int32_t>(100);
    std::shared_ptr<bool> hvi_bool = std::make_shared<bool>(true);
    std::shared_ptr<std::string> hvi_string = std::make_shared<std::string>("pippi");
    std::shared_ptr<fref> hvi_fref = std::make_shared<fref>(intsum);
    //var lvi_ptr *i32;            // allowed ?
    std::shared_ptr<sing::array<int32_t, 5>> hvi_array = std::make_shared<sing::array<int32_t, 5>>();
    *hvi_array = {1, 2, 3, 4, 5};
    std::shared_ptr<std::vector<int32_t>> hvi_dyna = std::make_shared<std::vector<int32_t>>();
    *hvi_dyna = {6, 7, 8, 9, 10};
    std::shared_ptr<std::vector<std::string>> hvi_stringarr = std::make_shared<std::vector<std::string>>();
    *hvi_stringarr = {"minni", "clara", "ciccio"};

    // pointers to heap-allocated vars
    std::shared_ptr<int32_t> pv_int32 = hv_int32;
    std::shared_ptr<bool> pv_bool = hv_bool;
    std::shared_ptr<std::string> pv_string = hv_string;
    std::shared_ptr<int32_t (*)(int32_t arg0, int32_t arg1, int32_t *arg2)> pv_fref = hv_fref;
    std::shared_ptr<std::shared_ptr<int32_t>> pv_ptr = hv_ptr;
    std::shared_ptr<sing::array<int32_t, 5>> pv_array = hv_array;
    std::shared_ptr<std::vector<int32_t>> pv_dyna = hv_dyna;
    std::shared_ptr<std::vector<std::string>> pv_stringarr = hv_stringarr;

    pv_int32 = hvi_int32;
    pv_bool = hvi_bool;
    pv_string = hvi_string;
    pv_fref = hvi_fref;
    //pv_ptr  = hvi_ptr;
    pv_array = hvi_array;
    pv_dyna = hvi_dyna;
    pv_stringarr = hvi_stringarr;

    // iterators and iterators' references: several addressings
    for(auto &intiterator : lvi_array) {
        ++intiterator;
    }
    for(auto &intiterator : *hvi_array) {
        ++intiterator;
    }
    for(auto &intiterator : *pv_array) {
        ++intiterator;
    }

    // iterators on several iterated types
    sing::array<bool, 3> boolvec = {false, false, true};
    for(auto &booliterator : boolvec) {
        booliterator = !booliterator;
    }

    for(auto &stringiterator : lvi_stringarr) {
        stringiterator += "!";
    }

    sing::array<std::shared_ptr<std::string>, 2> ptrvec = {hv_string, hvi_string};
    for(auto &ptriterator : ptrvec) {
        const std::shared_ptr<std::string> item = ptriterator;
        if (item != nullptr) {
            *item += "-";
        }
    }

    sing::array<sing::array<std::string, 3>, 3> arrvec = {{"a", "b", "c"}, {"d", "e"}, {"f", "g"}};
    for(auto &arrayiterator : arrvec) {
        for(auto &stringiterator : arrayiterator) {
            stringiterator += "_ok";
        }
    }

    std::vector<std::vector<std::string>> arrdyna = {{"a", "b", "c"}, {"d", "e"}, {"f", "g"}};
    for(auto &arrayiterator : arrdyna) {
        for(auto &stringiterator : arrayiterator) {
            stringiterator += "_ok";
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

    // comparisons
    const bool c0 = *hv_array == gvi_array;
    const bool c1 = *hv_dyna == gvi_dyna;

    // x-type assignment
    sing::copy_array_to_vec(*hv_dyna, gvi_array);

    // init of arrays with arrays
    const sing::array<int32_t, 5> newarray = *hv_array;
    const std::vector<int32_t> newvector = *hv_dyna;
    const sing::array<int32_t, 5> newvect2 = *hv_array;

    // passing arguments
    lv_int32 = gvi_fref(lv_int32, lvi_int32, &lv_int32);
    lv_int32 = intsum(lv_int32, lvi_int32, &lv_int32);
    lv_bool = boolsum(lv_bool, lvi_bool, &lv_bool);
    lv_string = stringsum(lv_string.c_str(), lvi_string.c_str(), &lv_string);
    gv_fref = frefshift(*hvi_fref, &lv_fref, &gvi_fref);
    arrayswap(lvi_array, &lv_array);
    dynaswap(lvi_dyna, &lv_dyna, 5);
    stringswap(*pv_stringarr, &lv_stringarr, 3);
}
