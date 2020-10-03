#include "sys_test.h"
#include "sys.h"
#include "sio.h"

bool sys_test()
{
    if (!testProcessFunctions()) {
        return (false);
    }

    if (!testTimeFunctions()) {
        return (false);
    }

    if (!testRndGen()) {
        return (false);
    }

    if (!testEnvir()) {
        return (false);
    }

    // mix
    sing::validate(true);

    // remark after test :-)
    //sys.validate(false);

    return (true);
}

bool testProcessFunctions()
{
    sing::system("C:/Windows/notepad.exe");

    uint64_t hh = sing::execute("C:/Windows/notepad.exe");
    if (sing::iseq(hh, 0)) {
        return (false);
    }
    sing::print("\nWaiting for notepad to exit");
    sing::waitCommandExit(hh);
    sing::print("\nnotepad exited");

    // automation test
    sing::iptr<sing::Stream> child_stdin;
    sing::iptr<sing::Stream> child_stdout;
    sing::iptr<sing::Stream> child_stderr;
    hh = sing::automate("C:/Windows/System32/choice.exe", &child_stdin, &child_stdout, &child_stderr);
    if (sing::iseq(hh, 0)) {
        return (false);
    }
    std::string prompt;
    if ((*child_stdout).gets(6, &prompt) != 0) {
        return (false);
    }
    if (prompt != "[Y,N]?") {
        return (false);
    }
    if ((*child_stdin).puts("Y") != 0) {
        return (false);
    }
    sing::waitCommandExit(hh);

    // remark after first test :-)
    // sys.exit(0);

    return (true);
}

bool testTimeFunctions()
{
    // timimngs
    const int64_t time_start = sing::time();
    const int64_t clock_start = sing::clock();
    sing::wait(2500000);
    const int64_t delta_time = sing::time() - time_start;
    const int64_t delta_clock = sing::clocksDiff(clock_start, sing::clock());
    if (delta_time < 2 || delta_time > 3 || delta_clock < 2400000 || delta_clock > 2600000) {
        return (false);
    }
    sing::BrokenTime btime;
    btime.fillLocal(sing::time());
    btime.fillUtc(sing::time());        // breakpoint here to check btime
    return (true);                      // breakpoint here to check btime
}

bool testRndGen()
{
    sing::RndGen gen;
    sing::array<int32_t, 100> lin_bukets = {0};
    sing::array<int32_t, 3> nor_bukets = {0};

    gen.rndSeed(100);
    if (gen.rndU64() == gen.rndU64()) {
        return (false);
    }

    // uniform generator
    for(int32_t count = 0; count < 100000; ++count) {
        ++lin_bukets[(int32_t)(gen.rnd() * 100.0)];
    }
    int32_t vmin = 1000000;
    int32_t vmax = 0;
    for(auto &val : lin_bukets) {
        vmin = std::min(vmin, val);
        vmax = std::max(vmax, val);
    }
    if (vmax > 1250 || vmin < 750) {
        return (false);
    }

    // normal generator
    for(int32_t count = 0; count < 10000; ++count) {
        ++nor_bukets[std::min(2, (int32_t)sing::abs(gen.rndNorm()))];
    }
    if (nor_bukets[0] * 27 > nor_bukets[1] * 75 || nor_bukets[0] * 27 < nor_bukets[1] * 60 || nor_bukets[0] * 4 > nor_bukets[2] * 75 ||
        nor_bukets[0] * 4 < nor_bukets[2] * 60) {
        return (false);
    }
    return (true);
}

bool testEnvir()
{
    // environment
    sing::setenv("myvar", "000", false);
    if (sing::getenv("myvar") != "000") {
        return (false);
    }
    sing::setenv("myvar", "001", false);
    if (sing::getenv("myvar") != "000") {
        return (false);
    }
    sing::setenv("myvar", "001");
    if (sing::getenv("myvar") != "001") {
        return (false);
    }
    return (true);
}
