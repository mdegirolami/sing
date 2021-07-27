#pragma once

#include <sing.h>

namespace sinth_test_oop {

// has constructor, destructor, private stuff
class stat final {
public:
    stat();
    ~stat();

    void add(float value);
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
    virtual void tough_test(bool enable) = 0;
};

// has virtual destructor, interface (support for typeswitch)
class c0_test final : public tough_tester {
public:
    c0_test();
    virtual ~c0_test();
    virtual void *get__id() const override { return(&id__); };
    void init();
    virtual void tough_test(bool enable) override;
    virtual result isgood() const override;

    static char id__;

private:
    std::string message_;
    bool istough_;
};

// testing 'by'
class delegating final : public tough_tester {
public:
    virtual void *get__id() const override { return(&id__); };
    void init()
    {
        implementor_.init();
    };
    virtual void tough_test(bool enable) override
    {
        implementor_.tough_test(enable);
    };
    virtual result isgood() const override
    {
        return(implementor_.isgood());
    };

    static char id__;
    std::weak_ptr<delegating> p1_;
    std::weak_ptr<delegating> p2_;

private:
    c0_test implementor_;
};

// very simple: NO costructor/destructor/private stuff/inheritance
class simple final {
public:
    std::string xxx_;
};

// how to implement inheritance
class if1 {
public:
    virtual ~if1() {}
    virtual void *get__id() const = 0;
    virtual void uno(int32_t a, int32_t b) const = 0;
    virtual void due(int32_t a, int32_t b) const = 0;
};

class if2 {
public:
    virtual ~if2() {}
    virtual void *get__id() const = 0;
    virtual void uno(int32_t a, int32_t b) const = 0;
    virtual void tre(float a, int32_t b) const = 0;
};

class Concrete final : public if1, public if2 {
public:
    virtual void *get__id() const override { return(&id__); };
    virtual void uno(int32_t a, int32_t b) const override;
    virtual void due(int32_t a, int32_t b) const override;
    virtual void tre(float a, int32_t b) const override;

    static char id__;
};

class Derived final : public if1, public if2 {
public:
    virtual void *get__id() const override { return(&id__); };
    void uno(int32_t a, int32_t b) const
    {
        embedded_.uno(a, b);
    };
    void due(int32_t a, int32_t b) const
    {
        embedded_.due(a, b);
    };
    virtual void tre(float a, int32_t b) const override;

    static char id__;

private:
    Concrete embedded_;
};

std::shared_ptr<delegating> test_oop();

extern const Concrete xxx;
extern const Derived xxy;

}   // namespace
