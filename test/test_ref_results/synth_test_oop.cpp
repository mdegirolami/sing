#include "synth_test_oop.h"

namespace sinth_test_oop {

// has virtual destructor, interface (support for typeswitch)
class c0_test final : public tough_tester {
public:
    c0_test();
    virtual ~c0_test();
    virtual void *get__id() const override { return(&id__); };
    void init();
    virtual void tough_test(const bool enable) override;
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
    virtual void tough_test(const bool enable) override
    {
        implementor_.tough_test(enable);
    };
    virtual result isgood() const override
    {
        return(implementor_.isgood());
    };

    static char id__;

private:
    c0_test implementor_;
};

typedef sing::map<std::string, int32_t> maptype;

static int32_t check_typeswitch(const tester &object);
static int32_t check_typeswitch2(const sing::iptr<tester> object);
static void check_builtin();

char c0_test::id__;
char delegating::id__;

//
// STAT class
//
stat::stat()
{
    count_ = 0;
    sum_ = 0;
    sum2_ = 0;
}

stat::~stat()
{
    count_ = 0;
}

void stat::add(const float value)
{
    ++count_;
    sum_ += value;
    sum2_ += value * value;
}

bool stat::getall(float *avg, float *variance) const
{
    *avg = this->avg();
    *variance = this->variance();
    return (true);
}

float stat::avg() const
{
    if (count_ == 0) {
        return ((float)0);
    } else {
        return (sum_ / (float)count_);
    }
}

float stat::variance() const
{
    if (count_ == 0) {
        return ((float)0);
    } else {
        const float avg = this->avg();
        return (sum2_ / (float)count_ - avg * avg);
    }
}

//
// c0_test
//
c0_test::c0_test()
{
    istough_ = false;
}

c0_test::~c0_test()
{
    message_ = "uninited";
}

void c0_test::init()
{
    message_ = "inited";
}

void c0_test::tough_test(const bool enable)
{
    istough_ = enable;
}

result c0_test::isgood() const
{
    if (istough_) {
        return (result::ok);
    }
    return (result::ko);
}

void test_oop()
{
    stat v_stat;
    float avg = 0;
    float variance = 0;

    // direct access
    v_stat.add((float)5);
    v_stat.add((float)10);
    v_stat.getall(&avg, &variance);

    sing::ptr<delegating> t_instance(new sing::wrapper<delegating>);
    const sing::iptr<tester> t_p = t_instance;

    // access through interface, switch integer constant
    switch ((*t_p).isgood()) {
    case result::ok: 
        avg *= 2.0f;
        break;
    case result::ko: 
        avg *= 0.5f;
        break;
    }

    switch (9) {
    case 9: 
        {
            v_stat.add(3.0f);
            v_stat.add(6.0f);
        }
        break;
    default:
        return;
        break;
    }

    // access through pointer
    const sing::ptr<delegating> t_p2 = t_instance;
    (*t_p2).tough_test(true);

    const c0_test alternate;

    check_typeswitch(alternate);
    check_typeswitch2(t_p);

    check_builtin();
}

static int32_t check_typeswitch(const tester &object)
{
    if (object.get__id() == &c0_test::id__) {
        return (0);                     // must select this
    } else if (object.get__id() == &delegating::id__) {
        delegating &ref = *(delegating *)&object;
        if (ref.isgood() == result::ok) {

            // before return1
            return (1);
        }
    } else {
        return (2);
    }
    return (-1);
}

static int32_t check_typeswitch2(const sing::iptr<tester> object)
{
    sing::ptr<delegating> tmp;
    if ((*object).get__id() == &c0_test::id__) {
        sing::ptr<c0_test> ref = (sing::wrapper<c0_test>*)object.get_wrapper();
        (*ref).tough_test(true);
        return (0);
    } else if ((*object).get__id() == &delegating::id__) {
        sing::ptr<delegating> ref = (sing::wrapper<delegating>*)object.get_wrapper();
        tmp = ref;                      // must select this
    } else {
        return (2);
    }
    return (-1);
}

static void check_builtin()
{
    int32_t sign = 0;
    const int32_t atan = 5;             // conflicts with functions ?
    int8_t int8 = (int8_t)-100;
    int8 = sing::abs(int8);
    int8 = (int8_t)::sqrt(int8);
    sign = sing::sgn(int8);

    int32_t int32 = -100;
    int32 = sing::abs(int32);
    int32 = (int32_t)::sqrt(int32);
    sign = sing::sgn(int32);

    uint64_t uint64 = (uint64_t)10000;
    uint64 = sing::abs(uint64);
    uint64 = (uint64_t)::sqrt(uint64);
    sign = sing::sgn(uint64);

    float f0 = -10000.0f;
    f0 = sing::abs(f0);
    f0 = (float)::sqrt(f0);
    sign = sing::sgn(f0);
    f0 = (float)::sin(f0);
    f0 = (float)::cos(f0);
    f0 = (float)::tan(f0);
    f0 = 0.5f;
    f0 = (float)::asin(f0);
    f0 = (float)::acos(f0);
    f0 = (float)::atan(f0);
    f0 = (float)::log(f0);
    f0 = (float)::exp(f0);
    f0 = (float)::log2(f0);
    f0 = (float)::exp2(f0);
    f0 = (float)::log10(f0);
    f0 = sing::exp10(f0);
    f0 = (float)::floor(f0);
    f0 = (float)::ceil(0.3f);
    f0 = (float)::round(-0.3f);
    f0 = (float)::round(-0.7f);

    double f1 = -10000.0;
    f1 = sing::abs(f1);
    f1 = ::sqrt(f1);
    sign = sing::sgn(f1);
    f1 = ::sin(f1);
    f1 = ::cos(f1);
    f1 = ::tan(f1);
    f1 = 0.5f;
    f1 = ::asin(f1);
    f1 = ::acos(f1);
    f1 = ::atan(f1);
    f1 = ::log(f1);
    f1 = ::exp(f1);
    f1 = ::log2(f1);
    f1 = ::exp2(f1);
    f1 = ::log10(f1);
    f1 = sing::exp10(f1);
    f1 = ::floor(f1);
    f1 = ::ceil(0.3);
    f1 = ::round(-0.3);
    f1 = ::round(-0.7);

    std::complex<float> cpl = -1.0f + std::complex<float>(0.0f, 1.0f);
    f0 = std::abs(cpl);
    f0 = std::arg(cpl);
    f0 = std::imag(cpl);
    f0 = std::real(cpl);
    f0 = std::norm(cpl);
    cpl = std::sqrt(cpl);
    cpl = std::sin(cpl);
    cpl = std::cos(cpl);
    cpl = std::tan(cpl);
    cpl = std::complex<float>(0.0f, 1.0f);
    cpl = std::asin(cpl);
    cpl = std::acos(cpl);
    cpl = std::atan(cpl);
    cpl = std::log(cpl);
    cpl = std::exp(cpl);

    std::vector<float> aa = {(float)1, (float)2, (float)3};
    aa.reserve(100);
    int32_t cc = aa.capacity();
    int32_t ss = aa.size();
    aa.shrink_to_fit();
    cc = aa.capacity();
    ss = aa.size();
    aa.resize(10);
    cc = aa.capacity();
    ss = aa.size();
    aa.clear();
    cc = aa.capacity();
    ss = aa.size();
    bool isempty = aa.empty();
    aa.push_back((float)5);
    aa.push_back((float)6);
    aa.pop_back();
    sing::insert(aa, 0, 5, (float)10);
    sing::erase(aa, 1, 4);
    std::vector<float> tt = aa;
    sing::insert_v(aa, 1, tt);
    sing::append(aa, tt);

    sing::ptr<std::vector<float>> bb(new sing::wrapper<std::vector<float>>);
    *bb = {(float)1, (float)2, (float)3};
    const sing::ptr<std::vector<float>> bbp = bb;
    (*bbp).push_back((float)1);
    ss = (*bb).size();

    // built-in on static vectors
    sing::array<std::string, 3> sv;
    ss = sv.size();

    // map constructors
    maptype map1;
    maptype map2 = {{"one", 1}, {"two", 2}, {"three", 3}};
    maptype map3 = map1;
    sing::ptr<maptype> map4(new sing::wrapper<maptype>);    // on heap with initializzation !
    *map4 = {{"one", 1}, {"two", 2}, {"three", 3}};
    const sing::ptr<maptype> mapp = map4;

    map1.reserve(100);
    int32 = map1.capacity();
    map1.insert("first", 10101);
    map1.insert("second", 89);
    map1.shrink_to_fit();
    int32 = map1.capacity();
    int32 = map1.size();
    bool test = map1.isempty();
    map1.clear();
    int32 = map1.size();
    test = map1.isempty();
    map2.erase("two");
    int32 = map2.get("one");
    int32 = map2.get_safe("one", -1);
    int32 = map2.get_safe("two", -1);
    test = map2.has("one");
    test = map2.has("two");
    std::string ts = map2.key_at(1);
    int32 = map2.value_at(1);
}

}   // namespace
