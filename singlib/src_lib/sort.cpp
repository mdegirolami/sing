#include <string.h>
//#include <string>
#include "sort.h"
#include "str.h"

namespace sing {

struct s_range
{
    s_range(int32_t b, int32_t c) { begin = b; count = c; } 
    int32_t begin;
    int32_t count;
};

static void quick_sort_recursive(int32_t *vv, size_t count, int depth, const Sortable &tosort);
static void quick_sort_stack(int32_t *vv, size_t count, const Sortable &tosort);
static size_t sort_around_pivot(int32_t *vv, size_t count, const Sortable &tosort);
static void merge_multiple(const int32_t *src, size_t v_size, size_t total_size, std::vector<int32_t> *dst, const Sortable &tosort);
static void merge(const int32_t *src0, size_t src0_size, const int32_t *src1, size_t src1_size, std::vector<int32_t> *dst, const Sortable &tosort);
static void bucket_sort(std::vector<int32_t> *index, std::vector<int32_t> *aux, const uint8_t *keys, size_t stride, size_t count, bool is_signed);

void indexInit(std::vector<int32_t> *index, const int32_t size)
{
    index->clear();
    index->reserve(size);
    for (int ii = 0; ii < size; ++ii) {
        index->push_back(ii);
    }
}

void qsort(std::vector<int32_t> *index, const Sortable &tosort)
{
    if (index->size() < 512) {
        quick_sort_recursive(index->data(), index->size(), 0, tosort);
    } else {
        quick_sort_stack(index->data(), index->size(), tosort);
    }
}

static void quick_sort_recursive(int32_t *vv, size_t count, int depth, const Sortable &tosort)
{
    // trivial cases
    if (count < 2) return;
    if (count == 2) {
        if (tosort.cmp(vv[0], vv[1]) > 0) {
            int32_t tmp = vv[1];
            vv[1] = vv[0];
            vv[0] = tmp;
        }
        return;
    }

    // if we are risking to blow the stack, Use another approach !!
    // if (depth > 200) {
    //     quick_sort_stack(vv, count, tosort);
    //     return;
    // }

    // sort around the pivot
    size_t pivot = sort_around_pivot(vv, count, tosort);

    // recur
    if (pivot > 1) {
        quick_sort_recursive(vv, pivot, depth + 1, tosort);
    }
    size_t tmp = count - pivot - 1;
    if (tmp > 1) {
        quick_sort_recursive(vv + (pivot + 1), tmp, depth + 1, tosort);
    }
}

static void quick_sort_stack(int32_t *vv, size_t count, const Sortable &tosort)
{
    std::vector<s_range> stack;

    stack.reserve(100);
    stack.emplace_back(0, count);

    while (!stack.empty()) {
        s_range rr = stack.back();
        stack.pop_back();
        if (rr.count > 1) {
            if (rr.count > 2) {
                size_t pivot = sort_around_pivot(vv + rr.begin, rr.count, tosort);
                stack.emplace_back(rr.begin, pivot);
                stack.emplace_back(rr.begin + pivot + 1, rr.count - pivot - 1);
            } else {
                if (tosort.cmp(vv[rr.begin], vv[rr.begin + 1]) > 0) {
                    std::swap(vv[rr.begin], vv[rr.begin + 1]);
                }              
            }
        }
    }
}

static size_t sort_around_pivot(int32_t *vv, size_t count, const Sortable &tosort)
{
    int32_t tmp;
    size_t lower = 0;
    size_t upper = count - 1;
    size_t pivot = count >> 1;
    while (true) {

        // find an item preceeding the pivot that should stay after the pivot.
        while (lower < pivot && tosort.cmp(vv[lower], vv[pivot]) <= 0) {
            ++lower;
        }

        // find an item succeeding the pivot that should stay before the pivot.
        while (upper > pivot && tosort.cmp(vv[upper], vv[pivot]) >= 0) {
            --upper;
        }

        // swap them
        if (lower < pivot) {
            if (upper > pivot) {
                tmp = vv[lower];
                vv[lower] = vv[upper];
                vv[upper] = tmp;
                ++lower;
                --upper;
            } else {

                // lower is out of place but not upper.
                // move the pivot down one position to make room for lower
                tmp = vv[pivot];
                vv[pivot] = vv[lower];
                vv[lower] = vv[pivot - 1];
                vv[pivot - 1] = tmp;
                --pivot;
                upper = pivot;
            }
        } else {
            if (upper > pivot) {

                // upper is out of place but not lower.
                tmp = vv[pivot];
                vv[pivot] = vv[upper];
                vv[upper] = vv[pivot + 1];
                vv[pivot + 1] = tmp;
                ++pivot;
                lower = pivot;
            } else {
                break;
            }
        }
    }
    return(pivot);
}

void msort(std::vector<int32_t> *index, const Sortable &tosort)
{
    std::vector<int32_t> aux;
    if (index->size() > 2) aux.reserve(index->size());
    for (size_t idx = 1; idx < index->size(); idx += 2) {
        if (tosort.cmp((*index)[idx-1], (*index)[idx]) > 0) {
            std::swap((*index)[idx-1], (*index)[idx]);
        }
    }
    size_t v_size = 2;
    while (v_size < index->size()) {
        aux.clear();
        merge_multiple(index->data(), v_size, index->size(), &aux, tosort);
        std::swap(*index, aux);
        v_size <<= 1;
    }
}

// the source is total_size long and split in internally ordered chunks, each one v_size long
// the destination must be made on chunks  v_size * 2 long
static void merge_multiple(const int32_t *src, size_t v_size, size_t total_size, std::vector<int32_t> *dst, const Sortable &tosort)
{
    size_t dst_size = v_size << 1;  // length of destination chunks
    const int32_t *src0 = src;
    const int32_t *src1 = src + v_size;
    size_t count;
    for (count = dst_size; count <= total_size; count += dst_size) {
        merge(src0, v_size, src1, v_size, dst, tosort);
        src0 += dst_size;
        src1 += dst_size;
    }
    size_t last_len = total_size - (count - dst_size);
    if (last_len > v_size) {
        merge(src0, v_size, src1, last_len - v_size, dst, tosort);
    } else {
        for (size_t idx = 0; idx < last_len; ++idx) {
            dst->push_back(*src0++);
        }
    }
}

static void merge(const int32_t *src0, size_t src0_size, const int32_t *src1, size_t src1_size, std::vector<int32_t> *dst, const Sortable &tosort)
{
    size_t scan0 = 0;
    size_t scan1 = 0;
    while (scan0 < src0_size && scan1 < src1_size) {
        if (tosort.cmp(src0[scan0], src1[scan1]) <= 0) {
            dst->push_back(src0[scan0++]);
        } else {
            dst->push_back(src1[scan1++]);
        }
    }
    while (scan0 < src0_size) {
        dst->push_back(src0[scan0++]);
    }
    while (scan1 < src1_size) {
        dst->push_back(src1[scan1++]);
    }
}

void ksort_u8(std::vector<int32_t> *index, const std::vector<uint8_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(uint8_t), index->size(), false);
}

void ksort_i8(std::vector<int32_t> *index, const std::vector<int8_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(int8_t), index->size(), true);
}

void ksort_u16(std::vector<int32_t> *index, const std::vector<uint16_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(uint16_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(uint16_t), index->size(), false);
}

void ksort_i16(std::vector<int32_t> *index, const std::vector<int16_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(int16_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(int16_t), index->size(), true);
}

void ksort_u32(std::vector<int32_t> *index, const std::vector<uint32_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(uint32_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(uint32_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 2, sizeof(uint32_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 3, sizeof(uint32_t), index->size(), false);
}

void ksort_i32(std::vector<int32_t> *index, const std::vector<int32_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(int32_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(int32_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 2, sizeof(int32_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 3, sizeof(int32_t), index->size(), true);
}

void ksort_u64(std::vector<int32_t> *index, const std::vector<uint64_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 2, sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 3, sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 4, sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 5, sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 6, sizeof(uint64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 7, sizeof(uint64_t), index->size(), false);
}

void ksort_i64(std::vector<int32_t> *index, const std::vector<int64_t> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 2, sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 3, sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 4, sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 5, sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 6, sizeof(int64_t), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 7, sizeof(int64_t), index->size(), true);
}

void ksort_f32(std::vector<int32_t> *index, const std::vector<float> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(float), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(float), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 2, sizeof(float), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 3, sizeof(float), index->size(), true);
}

void ksort_f64(std::vector<int32_t> *index, const std::vector<double> &keys)
{
    std::vector<int32_t> aux;
    aux.resize(index->size());
    bucket_sort(index, &aux, (const uint8_t *)keys.data(), sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 1, sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 2, sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 3, sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 4, sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 5, sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 6, sizeof(double), index->size(), false);
    bucket_sort(index, &aux, (const uint8_t *)keys.data() + 7, sizeof(double), index->size(), true);
}

void ksort_bool(std::vector<int32_t> *index, const std::vector<bool> &keys)
{
    std::vector<int32_t> aux;
    aux.reserve(index->size());
    size_t dst_false = 0;
    for (size_t ii = 0; ii < index->size(); ++ii) {
        int32_t idx = (*index)[ii];
        if (keys[idx]) {
            aux.push_back(idx);
        } else {
            (*index)[dst_false++] = idx;
        }
    }
    for (size_t ii = 0; ii < aux.size(); ++ii) {
        (*index)[dst_false++] = aux[ii];
    }
}

class istring_sorter : public Sortable {
private:
    const std::vector<std::string> *keys_;
public:
    virtual void *get__id() const { return(nullptr); }
    virtual int32_t cmp(const int32_t first, const int32_t second) const {
        return(compare((*keys_)[first].c_str(), (*keys_)[second].c_str(), true));
    }
    void setvector(const std::vector<std::string> *vv) { keys_ = vv; }
};

class string_sorter : public Sortable {
private:
    const std::vector<std::string> *keys_;
public:
    virtual void *get__id() const { return(nullptr); }
    virtual int32_t cmp(const int32_t first, const int32_t second) const {
        return(strcmp((*keys_)[first].c_str(), (*keys_)[second].c_str()));
    }
    void setvector(const std::vector<std::string> *vv) { keys_ = vv; }
};

void ksort_string(std::vector<int32_t> *index, const std::vector<std::string> &keys, bool insensitive)
{
    if (insensitive) {
        istring_sorter sorter;
        sorter.setvector(&keys);
        msort(index, sorter);
    } else {
        string_sorter sorter;
        sorter.setvector(&keys);
        msort(index, sorter);
    }
}

static void bucket_sort(std::vector<int32_t> *index, std::vector<int32_t> *aux, const uint8_t *keys, size_t stride, size_t count, bool is_signed)
{
    // how many of each kind ?
    size_t histo[256] = {0};
    size_t offset = 0;
    for (size_t idx = 0; idx < count; ++idx) {
        histo[keys[offset]]++;
        offset += stride;
    }

    // where to write each buket ?
    int32_t *dsts[256];
    int32_t *dst = aux->data();
    if (is_signed) {
        for (int ii = 128; ii < 256; ++ii) {
            dsts[ii] = dst;
            dst += histo[ii];
        }
        for (int ii = 0; ii < 128; ++ii) {
            dsts[ii] = dst;
            dst += histo[ii];
        }
    } else {
        for (int ii = 0; ii < 256; ++ii) {
            dsts[ii] = dst;
            dst += histo[ii];
        }
    }

    // distribute the element indices in the buckets
    for (size_t ii = 0; ii < count; ++ii) {
        int32_t idx = (*index)[ii];
        uint8_t key = keys[idx * stride];
        *dsts[key]++ = idx;
    }

    std::swap(*index, *aux);
}

} // namespace