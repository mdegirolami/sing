#include "sing.h"
#include "sort.h"
#include <stdlib.h>
#include <time.h>
#include <string>
#include <algorithm>
#include <string.h>

class string_sorter : public sing::Sortable {
private:
    const std::vector<std::string> *keys_;
public:
    virtual void *get__id() const { return(nullptr); }
    virtual int32_t cmp(const int32_t first, const int32_t second) const {
        return(strcmp((*keys_)[first].c_str(), (*keys_)[second].c_str()));
    }
    void setvector(const std::vector<std::string> *vv) { keys_ = vv; }
};

class int32_t_sorter : public sing::Sortable {
private:
    const std::vector<std::int32_t> *keys_;
public:
    virtual void *get__id() const { return(nullptr); }
    virtual int32_t cmp(const int32_t first, const int32_t second) const {
        int32_t ff = (*keys_)[first];
        int32_t ss = (*keys_)[second];
        if (ff > ss) return(1);
        if (ff < ss) return(-1);
        return(0);
    }
    void setvector(const std::vector<int32_t> *vv) { keys_ = vv; }
};

bool SortTest(int veclen)
{
    std::vector<int32_t> index;
    std::vector<int8_t> int8_keys;
    std::vector<uint8_t> uint8_keys;
    std::vector<int16_t> int16_keys;
    std::vector<uint16_t> uint16_keys;
    std::vector<int32_t> int32_keys;
    std::vector<uint32_t> uint32_keys;
    std::vector<int64_t> int64_keys;
    std::vector<uint64_t> uint64_keys;
    std::vector<float> float_keys;
    std::vector<double> double_keys;
    std::vector<bool> bool_keys;
    std::vector<std::string> string_keys;
    char to_add[11];
    string_sorter ss;
    int32_t_sorter is;
    std::vector<float> et;
    clock_t start;

    ss.setvector(&string_keys);
    is.setvector(&int32_keys);

    // fill the keys / indices
    for (int ii = 0; ii < veclen; ++ii) {
        int8_keys.push_back(rand());
        uint8_keys.push_back(rand());
        int16_keys.push_back(rand());
        uint16_keys.push_back(rand());
        int32_keys.push_back(rand() * rand());
        uint32_keys.push_back(rand() * rand());
        int64_keys.push_back((int64_t)rand() * rand() * rand() * rand());
        uint64_keys.push_back((uint64_t)rand() * rand() * rand() * rand());
        float_keys.push_back(sqrt(rand()));
        double_keys.push_back(sqrt(rand()));
        bool_keys.push_back((rand() & 1) != 0);
        for (int jj = 0; jj < 10; ++jj) {
            to_add[jj] = rand();
        }
        to_add[10] = 0;
        string_keys.push_back(to_add);
    }

    #ifdef NDEBUG
        int prescaler = 1000000 / veclen;
    #else
        int prescaler = 1;
    #endif

    // qsort int32_t
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::qsort(&index, is);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (int32_keys[index[ii]] < int32_keys[index[ii - 1]]) {
            return(false);
        }
    }

    // qsort string
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::qsort(&index, ss);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (string_keys[index[ii]] < string_keys[index[ii - 1]]) {
            return(false);
        }
    }

    // msort int32_t
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::msort(&index, is);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (int32_keys[index[ii]] < int32_keys[index[ii - 1]]) {
            return(false);
        }
    }

    // msort string
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::msort(&index, ss);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (string_keys[index[ii]] < string_keys[index[ii - 1]]) {
            return(false);
        }
    }

    // ksorts
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_u8(&index, uint8_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (uint8_keys[index[ii]] < uint8_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_i8(&index, int8_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (int8_keys[index[ii]] < int8_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_u16(&index, uint16_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (uint16_keys[index[ii]] < uint16_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_i16(&index, int16_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (int16_keys[index[ii]] < int16_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_u32(&index, uint32_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (uint32_keys[index[ii]] < uint32_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_i32(&index, int32_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (int32_keys[index[ii]] < int32_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_u64(&index, uint64_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (uint64_keys[index[ii]] < uint64_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_i64(&index, int64_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (int64_keys[index[ii]] < int64_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_f32(&index, float_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (float_keys[index[ii]] < float_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_f64(&index, double_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (double_keys[index[ii]] < double_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_bool(&index, bool_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (!bool_keys[index[ii]] && bool_keys[index[ii - 1]]) {
            return(false);
        }
    }

    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        sing::indexInit(&index, veclen);
        sing::ksort_string(&index, string_keys);
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));
    for (int ii = 1; ii < veclen; ++ii) {
        if (string_keys[index[ii]] < string_keys[index[ii - 1]]) {
            return(false);
        }
    }

    std::vector<int32_t> tosort;
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        tosort = int32_keys;
        std::sort(tosort.begin(), tosort.end());
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));

    std::vector<std::string> s_tosort;
    start = clock();
    for (int ii = 0; ii < prescaler; ++ii) {
        s_tosort = string_keys;
        std::sort(s_tosort.begin(), s_tosort.end());
    }
    et.push_back((clock() - start) * 1000.0f / (CLOCKS_PER_SEC * prescaler));

    #ifdef NDEBUG
        printf("\nqsort int = %g", et[0]);
        printf("\nqsort string = %g", et[1]);
        printf("\nmsort int = %g", et[2]);
        printf("\nmsort string = %g", et[3]);
        printf("\nksort u8 = %g", et[4]);
        printf("\nksort i8 = %g", et[5]);
        printf("\nksort u64 = %g", et[10]);
        printf("\nksort i64 = %g", et[11]);
        printf("\nksort bool = %g", et[14]);
        printf("\nksort string = %g", et[15]);
        printf("\nstd::sort int = %g", et[16]);
        printf("\nstd::sort string = %g", et[17]);
    #endif
    return(true);
}