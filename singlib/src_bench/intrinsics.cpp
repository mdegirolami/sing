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

    sing::dvect<int32_t> arr = {5, 6, 7};
    
    arr.reserve(100);
    test = arr.capacity();
    arr.trim();

    test = arr.size();
    arr.resize(100);
    arr.clear();
    bool empty = arr.isempty();
    arr.push_back(0);
    test = arr.last();
    arr.push_back(5);
    arr.pop_back();
    arr.insert(2, 10, 3);
    arr.erase(3, 8);
    sing::dvect<int32_t> arr2 = arr;
    arr.append(arr2);
    




// {"sort", "Mvxsf"}, {"radix_sort", "Mvxsf"}, {"find", "ie"}, {"binary_search", "ie"}, {"upper_bound", "ie"}, {"lower_bound", "ie"},
// {"", ""}

// {"reserve", "Mvs"}, {"capacity", "i"}, {"trim", "Mv"},
// {"clear" ,"Mv"}, {"size", "i"}, {"isempty", "b"},
// {"key_at", "kx"}, {"value_at", "vx"}, {"has", "bk"}, {"erase", "Mvk"},
// {"sort_by_key", "Mvxsf"}, {"radix_sort_by_key", "Mvxsf"}, {"sort_by_value", "Mvxsf"}, {"radix_sort_by_value", "Mvxsf"}, 
// {"find_value", "ie"}, {"binary_search_value", "ie"}, {"upper_bound_value", "ie"}, {"lower_bound_value", "ie"},
// {"", ""}
}

