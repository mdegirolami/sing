#include "sing.h"

void test_intrinsics(void)
{
    int64_t test = -0x100000000;

    test = sing::abs(test);
    test = (int64_t)sqrt(test);
    test = sing::sgn(test);

    float t2 = -1000.0f;
    t2 = sing::abs(t2);
    t2 = sqrt(t2);
    test = sing::sgn(t2);
    t2 = sin(t2);
    t2 = cos(t2);
    t2 = tan(t2);
    t2 = asin((float)0.5f);
    t2 = acos((float)0.5f);
    t2 = atan((float)0.5f);
    t2 = log(t2);
    t2 = exp(t2);
    t2 = log10(t2);
    t2 = sing::exp10(t2);
    t2 = log2(t2);
    t2 = exp2(t2);
    t2 = floor(t2) + 0.3f;
    t2 = ceil(t2) + 0.1f;
    t2 = round(t2);
    
    std::complex<float> t3(1.0f, 1.0f);

    t2 = abs(t3);
    t2 = arg(t3);
    t2 = imag(t3);
    t2 = real(t3);
    t2 = norm(t3);
    t3 = sin(t3);
    t3 = cos(t3);
    t3 = tan(t3);
    t3 = asin(t3);
    t3 = acos(t3);
    t3 = atan(t3);
    t3 = log(t3);
    t3 = exp(t3);
}

#ifndef NDEBUG

// remark in/out the refs. See how they cause exceptions.
void test_refguard(void)
{
    std::vector<int>    vv;
    vv.push_back(0);

    // build the pointer and guard
    int *ptr = &vv[0];
    //sing::Ref r0__("the ptr", ptr);

    // check construction/destruction
    {
        sing::Ref rx__("doesn't fire", ptr);
    }

    // build reference and guard
    int &ref = vv[0];
    //sing::Ref r1__("the ref", &ref);

    // invalidate ref/ptr
    vv.reserve(100);

    // see the effect without guards
    //int xx = *ptr;
    //*ptr = 0;


    std::string s0;

    // string pointer and guard
    const char *s0p = s0.c_str();
    //sing::Ref r2__("str0", s0p);

    // inavlidate
    sing::check_strrefs(s0.c_str());   // string needs explicit call
    s0 = "xxx";

    // what happens ?
    // xx = *s0p;

    // does the guard catch ptr deallocated stuff ?
    std::shared_ptr<int> heapstuff = std::make_shared<int>(5);
    sing::Ref r3__("heap ptr", &*heapstuff);
    heapstuff = nullptr;
}

#endif
