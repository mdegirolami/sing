#pragma once

#include <sing.h>

namespace sing {

class Sortable {
public:
    virtual ~Sortable() {}
    virtual void *get__id() const = 0;
    virtual int32_t cmp(int32_t first, int32_t second) const = 0;
};

void indexInit(std::vector<int32_t> *index, int32_t size);
void qsort(std::vector<int32_t> *index, const Sortable &tosort);
void msort(std::vector<int32_t> *index, const Sortable &tosort);
void ksort_u8(std::vector<int32_t> *index, const std::vector<uint8_t> &keys);
void ksort_i8(std::vector<int32_t> *index, const std::vector<int8_t> &keys);
void ksort_u16(std::vector<int32_t> *index, const std::vector<uint16_t> &keys);
void ksort_i16(std::vector<int32_t> *index, const std::vector<int16_t> &keys);
void ksort_u32(std::vector<int32_t> *index, const std::vector<uint32_t> &keys);
void ksort_i32(std::vector<int32_t> *index, const std::vector<int32_t> &keys);
void ksort_u64(std::vector<int32_t> *index, const std::vector<uint64_t> &keys);
void ksort_i64(std::vector<int32_t> *index, const std::vector<int64_t> &keys);
void ksort_f32(std::vector<int32_t> *index, const std::vector<float> &keys);
void ksort_f64(std::vector<int32_t> *index, const std::vector<double> &keys);
void ksort_bool(std::vector<int32_t> *index, const std::vector<bool> &keys);
void ksort_string(std::vector<int32_t> *index, const std::vector<std::string> &keys, bool insensitive = false);

}   // namespace
