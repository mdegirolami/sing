#pragma once

#include <sing.h>
#include "sio.h"

namespace sing {

// processes
typedef uint64_t Phandle;

class BrokenTime final {
public:
    BrokenTime();

    void fillLocal(const int64_t time);
    void fillUtc(const int64_t time);

    int8_t second_;                     // seconds after the minute — [0, 60]
    int8_t minute_;                     // minutes after the hour — [0, 59]
    int8_t hour_;                       // hours since midnight — [0, 23]
    int8_t mday_;                       // day of the month — [1, 31]
    int8_t mon_;    // months since January — [0, 11]
    int8_t wday_;                       // days since Sunday — [0, 6]
    int16_t yday_;                      // days since January 1 — [0, 365]
    bool savings_;                      // Daylight Saving Time flag
    int32_t year_;                      // years since 1900
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
