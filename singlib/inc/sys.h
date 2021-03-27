#pragma once

#include <sing.h>
#include "sio.h"

namespace sing {

// processes
typedef uint64_t Phandle;

class BrokenTime final {
public:
    BrokenTime();

    void fillLocal(int64_t time);
    void fillUtc(int64_t time);

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
    void rndSeed(int64_t seed);
    uint64_t rndU64();
    double rnd();
    double rndNorm();

private:
    int64_t seed_;
};

enum class OsId {win, linux, osx};

int32_t system(const char *command);
Phandle execute(const char *command);
Phandle automate(const char *command, std::shared_ptr<Stream> *sstdin, std::shared_ptr<Stream> *sstdout, std::shared_ptr<Stream> *sstderr);
int32_t waitCommandExit(Phandle handle);
void exit(int32_t retcode);

// time management
void wait(int32_t microseconds);
int64_t time();     // seconds resolution
int64_t clock();
int64_t clocksDiff(int64_t before, int64_t after);          // microseconds

// environment
std::string getenv(const char *name);
void setenv(const char *name, const char *value, bool override = true);

// mix
void validate(bool condition);

OsId getOs();

}   // namespace
