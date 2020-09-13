#pragma once

#include <sing.h>
#include "sio.h"

namespace sing {

// processes
typedef uint64_t Phandle;

class BrokenTime final {                // seconds after the minute — [0, 60]
public:             // minutes after the hour — [0, 59]
    BrokenTime();   // hours since midnight — [0, 23]
    void fillLocal(const int64_t time); // day of the month — [1, 31]
    void fillUtc(const int64_t time);   // months since January — [0, 11]

    int8_t second_; // days since Sunday — [0, 6]
    int8_t minute_; // days since January 1 — [0, 365]
    int8_t hour_;   // Daylight Saving Time flag
    int8_t mday_;   // years since 1900
    int8_t mon_;
    int8_t wday_;
    int16_t yday_;
    bool savings_;
    int32_t year_;
};

// random numbers
class RndGen final {
public:
    RndGen();
    void rndSeed(const int64_t seed);
    uint64_t rndU64();
    double rnd();
    double rndNorm();

private:
    int64_t seed_;
};

void system(const char *command);
Phandle execute(const char *command);
Phandle automate(const char *command, sing::iptr<Stream> *sstdin, sing::iptr<Stream> *sstdout, sing::iptr<Stream> *sstderr);
int32_t waitCommandExit(const Phandle handle);
void exit(const int32_t retcode);

// time management
void wait(const int32_t microseconds);
int64_t time();     // seconds resolution
int64_t clock();
int64_t clocksDiff(const int64_t before, const int64_t after);                  // microseconds

// environment
std::string getenv(const char *name);
void setenv(const char *name, const char *value, const bool override = true);

// mix
void validate(const bool condition);

}   // namespace
