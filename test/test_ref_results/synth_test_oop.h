#pragma once

#include <sing.h>

namespace sinth_test_oop {

// has constructor, destructor, private stuff
class stat final {
public:
    stat();
    ~stat();
    void add(const float value);
    bool getall(float *avg, float *variance) const;

    int32_t count_;

private:
    float avg() const;
    float variance() const;

    float sum_;
    float sum2_;
};


// some interfaces
enum class result {ok = 1, ko};

class tester {
public:
    virtual ~tester() {}
    virtual void *get__id() const = 0;
    virtual result isgood() const = 0;
};

class tough_tester : public tester {
public:
    virtual void tough_test(const bool enable) = 0;
};

// very simple: NO costructor/destructor/private stuff/inheritance
class simple final {
public:
    std::string xxx_;
};

void test_oop();

}   // namespace
