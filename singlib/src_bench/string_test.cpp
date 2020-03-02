#include <float.h>
#include "sing.h"

static const sing::string gs0("global");
static sing::string gs1("global");

void string_receiver(const sing::string &in_string, sing::string *out_string)
{
    (*out_string) = sing::add_strings(2, in_string.c_str(), in_string.c_str());
}

void string_vect_receiver(const sing::vect<sing::string> &in_string, sing::vect<sing::string> *out_string)
{
    (*out_string)[0] = sing::add_strings(2, in_string[0].c_str(), in_string[1].c_str());
}

void using_strings(void)
{
    const sing::string ls0 = "local const";
    sing::string ls1("local");

    const sing::string pippoc = ls1 + "bbb";
    sing::string pippo = "bbb" + ls1;

    pippo = ls1 + 'a';
    pippo = 'a' + ls1;

    sing::ptr<sing::string> ds0(new sing::wrapper<sing::string>);
    *ds0 = "xxx";

    sing::dvect<sing::string> vs0 = { "a", "b", "c" };
    sing::dvect<sing::string> vs1 { "a", "b", "c" };

    ls1 += ls0;
    vs1[2] = "kk";

    sing::string *str__it;
    for (str__it = vs0.begin(); str__it < vs0.end(); ++str__it) {
        *str__it = "x";
    }

    int32_t v0 = (int32_t)sing::string2int("100");
    int32_t v1 = (int32_t)sing::string2int(gs1);
    uint32_t uv0 = (uint32_t)sing::string2uint("100");
    uint32_t uv1 = (uint32_t)sing::string2uint(gs1);
    float f0 = (float)sing::string2double("100");
    float f1 = (float)sing::string2double(gs1);
    std::complex<float> c0 = sing::string2complex64("100 + 5i");
    std::complex<double> c1 = sing::string2complex128("100 + 5i");

    gs1 = sing::tostring((int8_t)100);
    gs1 = sing::tostring((int16_t)100);
    gs1 = sing::tostring((int32_t)-(1 << 31));
    gs1 = sing::tostring((int64_t)0x8000000000000000);
    gs1 = sing::tostring((uint8_t)100);
    gs1 = sing::tostring((uint16_t)100);
    gs1 = sing::tostring((uint32_t)100);
    gs1 = sing::tostring((uint64_t)100);
    gs1 = sing::tostring(100.12f);
    gs1 = sing::tostring(-DBL_MAX);
    gs1 = sing::tostring(std::complex<float>(67, 23.2f));
    gs1 = sing::tostring(std::complex<double>(-DBL_MAX, -DBL_MAX));
    gs1 = sing::tostring(false);

    gs1 = sing::format("ds Ds us Us cs Cs fs ss bs rsR", 
        0x80000000, " ", 0x8000000000000000, " ", 0x80000000, " ", 0x8000000000000000, " ",
        1280, " ", 1280ll, " ", -DBL_MAX, " ", pippo.c_str(), " ", false, " ", c0, " ", c1);

    bool cmp = gs1 > ls1;
    cmp = gs1 == ls1;
    cmp = gs1 == gs1;
    cmp = "kkk" < gs1;

    // function calls
    string_receiver(gs0, &gs1);
    string_receiver(ls0, &ls1);
    string_receiver(pippo, ds0);
    string_receiver((*ds0), &vs1[0]);
    string_receiver(vs0[0], &vs1[0]);
    string_receiver("boia", &vs1[0]);
    string_vect_receiver(vs0, &vs1);
}
