#include "synth_pkgs_test.h"
#include "assert.h"
#include "synth_test_pkg.h"

typedef sing::ptr<aa::bb::pkg_type> gg; // good !!

static void test();

static sing::array<int32_t, aa::bb::pkg_ctc> vf;            // good !!

const std::complex<double> kk = std::complex<double>(100.3, 12.0);
const sing::array<int32_t, 3> table = {100, 200, 300};

static void test()
{
    aa::bb::pkg_fun(vf);
}

// public inclusion of synth_test_pkg2
void test2(const pkg_type2 p0)
{
    aa::bb::pkg_type tt = 0;            // needed by a public, but not in the declaration: private inclusion.

    assert(true);
}
