#include "synth_pkgs_test.h"
#include "assert.h"
#include "synth_test_pkg.h"

typedef aa::bb::pkg_type gg;

void test();

static sing::spvect<int32_t, aa::bb::pkg_ctc> vf;
const std::complex<double> kk = std::complex<double>(100.3, 12.0);
const sing::spvect<int32_t, 3> table = {100, 200, 300};


void test()
{
    aa::bb::pkg_fun(vf);
}

void test2(const pkg_type2 p0)
{
    aa::bb::pkg_type tt = 0;

    assert(true);
}
