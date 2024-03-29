#include <string.h>
//#include <string>
#include "str.h"
#include "str_tables.h"

// NOTE:
// to be added to the automatically generated File class declaration: 
/*
int32_t decode_or_skip(const char **src);
inline int32_t toCp(const char *src) 
{
    if ((*src & 0x80) == 0) {
        return(*src);
    } else {
        const char *scan = src;
        return(decode_or_skip(&scan));
    }
}
*/

namespace sing {

Range::Range()
{
    begin_ = end_ = npos;    
}

// local functions
static void cp_encode(std::string *dst, int32_t cp);
static int32_t cp_decode_impl(const char **src);
static bool split_impl(const char *src, const Range &range, std::string *left, std::string *right,  SplitMode mode);
static const char *skip_chars_forward(const char *start, int32_t num_chars);
static const char *skip_chars_reverse(const char *start, int32_t num_chars, const char *base);
static bool is_good_cp(const char *src);

inline bool starting(const char *pos) 
{
    return((*pos & 0x80) == 0 || ((unsigned int)*pos) >= 0xc0 && is_good_cp(pos));
}

inline int32_t cp_decode(const char **src)
{
    if ((**src & 0x80) == 0) {
        if (**src == 0) return(0);
        return(*(*src)++);
    } else {
        return(cp_decode_impl(src));
    }
}

// implementation of public functions

int32_t len(const char *src)
{
    return(strlen(src));
}

int32_t numchars(const char *src)
{    
    if (src == nullptr) return(0);
    int count = 0;
    while (*src != 0) {
        if (starting(src++)) ++count;
    }
    return(count);
}

int32_t count(const char *src, const char *to_search, const bool insensitive)
{
    Range range;

    if (src == nullptr || to_search == nullptr) return(npos);
    int count = 0;
    int pos = 0;
    while (true) {
        if (!find(src, to_search, &range, insensitive, pos)) {
            return(count);
        }
        ++count;
        pos = range.end_;
    }
}

std::string toupper(const char *src)
{
    std::string dst;
    dst.reserve((strlen(src) * 17) >> 4);
    while (*src != 0) {
        if ((*src & 0x80) == 0) {
            if (*src >= 'a' && *src <= 'z') {
                dst += *src + ('A' - 'a');
            } else {
                dst += *src;
            }
            ++src;
        } else {
            cp_encode(&dst, ccToupper(cp_decode_impl(&src)));
        }
    }
    return(dst);
}

std::string tolower(const char *src)
{
    std::string dst;
    dst.reserve((strlen(src) * 17) >> 4);
    while (*src != 0) {
        if ((*src & 0x80) == 0) {
            if (*src >= 'A' && *src <= 'Z') {
                dst += *src + ('a' - 'A');
            } else {
                dst += *src;
            }
            ++src;
        } else {
            cp_encode(&dst, ccTolower(cp_decode_impl(&src)));
        }
    }
    return(dst);
}

std::string totitle(const char *src)
{
    std::string dst;
    dst.reserve((strlen(src) * 17) >> 4);
    while (*src != 0) {
        if ((*src & 0x80) == 0) {
            if (*src >= 'a' && *src <= 'z') {
                dst += *src + ('A' - 'a');
            } else {
                dst += *src;
            }
            ++src;
        } else {
            cp_encode(&dst, ccTotitle(cp_decode_impl(&src)));
        }
    }
    return(dst);
}

int32_t compare(const char *first, const char *second, const bool insensitive)
{
    if (!insensitive) {
        return(strcmp(first, second));
    }
    while (*first != 0 && *second != 0) {
        int32_t cpf = ccToupper(cp_decode(&first));
        int32_t cps = ccToupper(cp_decode(&second));
        if (cpf != cps) return(cpf - cps);
    }
    return(*first - *second);
}

int32_t compareAt(const char *first, const int32_t at_pos, const char *contained, int32_t *end_pos, const bool insensitive)
{
    const char *scan = first + at_pos;
    if (!insensitive) {
        while (*scan != 0 && *contained != 0) {
            int32_t cpf = cp_decode(&scan);
            int32_t cps = cp_decode(&contained);
            if (cpf != cps) {
                *end_pos = scan - first;
                return(cpf - cps);
            }
        }
    } else {
        while (*scan != 0 && *contained != 0) {
            int32_t cpf = ccToupper(cp_decode(&scan));
            int32_t cps = ccToupper(cp_decode(&contained));
            if (cpf != cps) {
                *end_pos = scan - first;
                return(cpf - cps);
            }
        }
    }
    *end_pos = scan - first;
    if (*contained == 0) return(0);
    return(*scan - *contained);
}

bool split(const char *src, const char *splitter, 
           std::string *left, std::string *right, SplitMode mode, const bool insensitive)
{
    Range range;
    find(src, splitter, &range, insensitive);
    return(split_impl(src, range, left, right, mode));    
}

bool splitAny(const char *src, const char *splitter, 
           std::string *left, std::string *right, SplitMode mode, const bool insensitive)
{
    Range range;
    findAny(src, splitter, &range, insensitive);
    return(split_impl(src, range, left, right, mode));    
}

bool rsplit(const char *src, const char *splitter, 
           std::string *left, std::string *right, SplitMode mode, const bool insensitive)
{
    Range range;
    rfind(src, splitter, &range, insensitive);
    return(split_impl(src, range, left, right, mode));    
}

bool rsplitAny(const char *src, const char *splitter, 
           std::string *left, std::string *right, SplitMode mode, const bool insensitive)
{
    Range range;
    rfindAny(src, splitter, &range, insensitive);
    return(split_impl(src, range, left, right, mode));    
}

static bool split_impl(const char *src, const Range &range, std::string *left, std::string *right, SplitMode mode)
{
    if (range.begin_ == npos) return(false);
    int first_end = mode == SplitMode::sm_separator_left ? range.end_ : range.begin_;
    int second_begin = mode == SplitMode::sm_separator_right ? range.begin_ : range.end_;
    if (src == left->c_str()) {
        *right = std::string(src + second_begin);
        left->resize(first_end);
    } else if (src == right->c_str()) {
        *left = std::string(src, first_end);
        right->erase(0, second_begin);
    } else {
        *left = std::string(src, first_end);
        *right = std::string(src + second_begin);
    }
    return(true);
}

int32_t replace(std::string *src, const char *old_sub, const char *new_sub, const bool insensitive, const int32_t from)
{
    Range range;
    if (!find(src->c_str(), old_sub, &range, insensitive, from)) {
        return(npos);
    }
    int old_len = range.end_ - range.begin_;
    int new_len = strlen(new_sub);
    for (int ii = 0; ii < new_len && ii < old_len; ++ii) {
        (*src)[range.begin_ + ii] = new_sub[ii];
    }
    if (new_len > old_len) {
        src->insert(range.begin_ + old_len, new_sub + old_len);
    } else {
        if (new_len < old_len) {
            src->erase(range.begin_ + new_len, old_len - new_len);        
        }
    }
    return(range.end_);
}

int32_t replaceAll(std::string *src, const char *old_sub, const char *new_sub, const bool insensitive)
{
    Range range;
    int new_len = strlen(new_sub);

    // detect (and count) the replacement positions 
    std::vector<Range> all_pos;
    int32_t pos = 0;
    while (find(src->c_str(), old_sub, &range, insensitive, pos)) {
        all_pos.push_back(range);
        pos = range.end_;
    }

    // select a replacement algo.
    bool new_is_smaller = true;
    bool new_is_equal = true;
    int  to_delete_total = 0;
    for (int ii = 0; ii < all_pos.size(); ++ii) {
        int len = all_pos[ii].end_ - all_pos[ii].begin_;
        to_delete_total += len;
        if (new_is_smaller && new_len > len) {
            new_is_smaller = false;
        }
        if (new_is_equal && new_len != len) {
            new_is_equal = false;
        }
    }

    if (new_is_equal) {
        for (int ii = 0; ii < all_pos.size(); ++ii) {
            int dst = all_pos[ii].begin_;
            for (const char *scan = new_sub; *scan != 0; ++scan) {
                (*src)[dst++] = *scan;
            }
        }
    } else if (new_is_smaller) {
        int dstidx = 0;
        int srcidx = 0;

        // self-copy to the buffer the fragment between the replace points and the new_sub instances
        for (int ii = 0; ii < all_pos.size(); ++ii) {
            Range *pr = &all_pos[ii];
            int top = pr->begin_;
            while (srcidx != top) {
                (*src)[dstidx++] = (*src)[srcidx++];
            }
            for (const char *scan = new_sub; *scan != 0; ++scan) {
                (*src)[dstidx++] = *scan;
            }
            srcidx = pr->end_;
        }
        while ((*src)[srcidx] != 0) {
            (*src)[dstidx++] = (*src)[srcidx++];
        }
        src->resize(dstidx);
    } else {
        // allocate an appropriate sized buffer
        std::string buffer;
        buffer.reserve(src->length() - to_delete_total + new_len * all_pos.size());    

        // copy to the buffer the fragment between the replace points and the new_sub instances
        // note: copied_to marks the last read from the original string.
        int copied_to = 0;
        for (int ii = 0; ii < all_pos.size(); ++ii) {
            Range *pr = &all_pos[ii];
            int count = pr->begin_ - copied_to;
            if (count > 0) {
                buffer.append(src->c_str() + copied_to, count);
                copied_to += count;
            }
            buffer.append(new_sub);
            copied_to = pr->end_;
        }
        buffer.append(src->c_str() + copied_to);

        // copy the buffer to the original string and get rid of it.
        // (is the compiler smart enough to use std::move since buffer is exiting the scope ?)
        *src = buffer;
    }
    return(all_pos.size());
}

bool hasPrefix(const char *src, const char *prefix, const bool insensitive)
{
    int32_t end;
    return(compareAt(src, 0, prefix, &end, insensitive) == 0);
}

bool hasSuffix(const char *src, const char *suffix, const bool insensitive)
{
    int32_t end;
    return(compareAt(src, skipBkw(src, npos, numchars(suffix)), suffix, &end, insensitive) == 0);
}

void cutPrefix(std::string *str, const char *prefix, const bool insensitive)
{
    int32_t end;
    if (compareAt(str->c_str(), 0, prefix, &end, insensitive) == 0) {
        str->erase(0, end);
    } 
}

void cutSuffix(std::string *str, const char *suffix, const bool insensitive)
{
    int32_t end;
    int32_t suffix_pos = skipBkw(str->c_str(), npos, numchars(suffix));
    if (compareAt(str->c_str(), suffix_pos, suffix, &end, insensitive) == 0) {
        str->resize(suffix_pos);
    }
}

void cutLeadingSpaces(std::string *str)
{
    const char *base = str->c_str();
    const char *src = base;
    const char *cutpoint = base;
    int32_t cp = 0;
    do {
        cutpoint = src;
        cp = cp_decode(&src);
        if (cp == 0) {
            *str = "";
            return;
        }
    } while (isSpace(cp));
    int to_erase = cutpoint - base;
    if (to_erase > 0) {
        str->erase(0, to_erase);
    }
}

void cutTrailingSpaces(std::string *str)
{
    int top = str->length() - 1;
    int idx = 0;
    const char *src = nullptr;

    for (idx = top; idx >= 0; --idx) {        
        src = str->c_str() + idx;
        if (starting(src)) {
            if (!isSpace(cp_decode(&src))) break;
        }
    }
    if (idx < 0) {
        *str = "";
    } else {
        str->resize(src - str->c_str());
    }
}

void cutLeading(std::string *str, const Selector &to_keep)
{
    const char *base = str->c_str();
    const char *src = base;
    const char *cutpoint = base;
    int32_t cp = 0;
    do {
        cutpoint = src;
        cp = cp_decode(&src);
        if (cp == 0) {
            *str = "";
            return;
        }
    } while (!to_keep.isGood(cp));
    int to_erase = cutpoint - base;
    if (to_erase > 0) {
        str->erase(0, to_erase);
    }
}

void cutTrailing(std::string *str, const Selector &to_keep)
{
    int top = str->length() - 1;
    int idx = 0;
    const char *src = nullptr;

    for (idx = top; idx >= 0; --idx) {        
        src = str->c_str() + idx;
        if (starting(src)) {
            int32_t cp = cp_decode(&src);
            if (to_keep.isGood(cp)) break;
        }
    }
    if (idx < 0) {
        *str = "";
    } else {
        str->resize(src - str->c_str());
    }
}

void cutFun(std::string *str, const Selector &to_keep)
{
    const char *src = str->c_str();
    std::string dst;
    int32_t cp = 0;
    do {
        cp = cp_decode(&src);
        if (to_keep.isGood(cp)) {
            cp_encode(&dst, cp);
        }
    } while (cp != 0);
    *str = dst;
}

void makeUtf8Compliant(std::string *str)
{
    const char *src = str->c_str();
    std::string dst;
    int32_t cp = 0;
    do {
        cp = cp_decode(&src);
        cp_encode(&dst, cp);
    } while (cp != 0);
    *str = dst;
}

bool find(const char *src, const char *to_search, Range *range, const bool insensitive, const int32_t from)
{
    for (int idx = std::max(from, 0); src[idx] != 0; ++idx) {
        if (starting(src + idx)) {
            if (compareAt(src, idx, to_search, &range->end_, insensitive) == 0) {
                range->begin_ = idx;
                return(true);
            }
        }
    }
    return(false);
}

// bool find(const char *src, const char *to_search, Range *range, const bool insensitive, const int32_t from)
// {
//     if (!insensitive) {
//         for (int idx = std::max(from, 0); src[idx] != 0; ++idx) {
//             if (starting(src+idx)) {
//                 const char *scan = src + idx;
//                 const char *contained = to_search;
//                 while (*scan != 0 && *contained != 0) {
//                     int32_t cpf = cp_decode(&scan);
//                     int32_t cps = cp_decode(&contained);
//                     if (cpf != cps) break;
//                 }
//                 if (*contained == 0) {
//                     range->begin_ = idx;
//                     range->end_ = scan - src;
//                     return(true);
//                 }
//             }
//         }
//         return(false);
//     } else {
//         for (int idx = std::max(from, 0); src[idx] != 0; ++idx) {
//             if (starting(src+idx)) {
//                 const char *scan = src + idx;
//                 const char *contained = to_search;
//                 while (*scan != 0 && *contained != 0) {
//                     int32_t cpf = ccToupper(cp_decode(&scan));
//                     int32_t cps = ccToupper(cp_decode(&contained));
//                     if (cpf != cps) break;
//                 }
//                 if (*contained == 0) {
//                     range->begin_ = idx;
//                     range->end_ = scan - src;
//                     return(true);
//                 }
//             }
//         }
//         return(false);
//     }
// }

bool rfind(const char *src, const char *to_search, Range *range, const bool insensitive, const int32_t from)
{
    int32_t top;

    if (from == npos) {
        top = (int32_t)strlen(src);
    } else {
        top = from;
    }
    for (int32_t idx = top - 1; idx >= 0; --idx) {
        if (starting(src + idx)) {
            if (compareAt(src, idx, to_search, &range->end_, insensitive) == 0) {
                if (range->end_ <= top) {
                    range->begin_ = idx;
                    return(true);
                }
            }
        }
    }
    return(false);
}

bool findAny(const char *src, const char *to_search, Range *range, const bool insensitive, const int32_t from)
{
    std::vector<int32_t>    to_match;

    decode(to_search, &to_match);    
    if (insensitive) {
        for (int ii = 0; ii < to_match.size(); ++ii) {
            to_match[ii] = ccToupper(to_match[ii]);
        }
    }
    for (int32_t idx = std::max(from, 0); src[idx] != 0; ++idx) {
        const char *fd = src + idx;
        if (starting(fd)) {
            int32_t cp = cp_decode(&fd);
            if (insensitive) {
                cp = ccToupper(cp);
            }
            for (int32_t idx2 = 0; idx2 < to_match.size(); ++idx2) {            
                if (cp == to_match[idx2]) {
                    range->begin_ = idx;
                    range->end_ = fd - src;
                    return(true);
                }
            }
        }
    }
    return(false);
}

bool rfindAny(const char *src, const char *to_search, Range *range, const bool insensitive, const int32_t from)
{
    std::vector<int32_t>    to_match;
    int32_t                 top;

    decode(to_search, &to_match);    
    if (insensitive) {
        for (int ii = 0; ii < to_match.size(); ++ii) {
            to_match[ii] = ccToupper(to_match[ii]);
        }
    }
    if (from == npos) {
        top = (int32_t)strlen(src);
    } else {
        top = from;
    }
    for (int32_t idx = top - 1; idx >= 0; --idx) {
        const char *fd = src + idx;
        if (starting(fd)) {
            int32_t cp = cp_decode(&fd);
            if (insensitive) {
                cp = ccToupper(cp);
            }
            for (int32_t idx2 = 0; idx2 < to_match.size(); ++idx2) {            
                if (cp == to_match[idx2]) {
                    range->begin_ = idx;
                    range->end_ = fd - src;
                    return(true);
                }
            }
        }
    }
    return(false);
}

bool findFnc(const char *src, const Selector &matches, Range *range, const int32_t from)
{
    for (int32_t idx = std::max(from, 0); src[idx] != 0; ++idx) {
        const char *fd = src + idx;
        if (starting(fd)) {
            int32_t cp = cp_decode(&fd);
            if (matches.isGood(cp)) {
                range->begin_ = idx;
                range->end_ = fd - src;
                return(true);
            }
        }
    }
    return(false);    
}

bool rfindFnc(const char *src, const Selector &matches, Range *range, const int32_t from)
{
    int32_t top;

    if (from == npos) {
        top = (int32_t)strlen(src);
    } else {
        top = from;
    }
    for (int32_t idx = top - 1; idx >= 0; --idx) {
        const char *fd = src + idx;
        if (starting(fd)) {
            int32_t cp = cp_decode(&fd);
            if (matches.isGood(cp)) {
                range->begin_ = idx;
                range->end_ = fd - src;
                return(true);
            }
        }
    }
    return(false);    
}

std::string sub(const char *src, const int32_t first, const int32_t last)
{
    int32_t ff = std::max(0, first);    
    if (last <= ff && last != npos) return(std::string());
    if (last == npos) return(std::string(src+ff));
    return(std::string(src + ff, last - ff));
}

std::string subRange(const char *src, const Range &range)
{
    return(sub(src, range.begin_, range.end_));
}

int32_t pos2idx(const char *src, const int32_t pos)
{
    int found = 0;
    const char *scan;
    for (scan = src; *scan != 0; ++scan) {
        if (starting(scan)) {
            if (found == pos) {
                return(scan - src);
            }
            ++found;
        }
    }
    return(found == pos ? scan - src : npos);
}

int32_t idx2pos(const char *src, const int32_t idx)
{
    const char *top = src + idx;
    if (!starting(top)) {
        return(npos);
    }
    int found = 0;
    const char *scan;
    for (scan = src; *scan != 0 && scan < top; ++scan) {
        if (starting(scan)) {
            ++found;
        }
    }
    return(scan == top ? found : npos);
}

void insert(std::string *str, const int32_t idx, const char *to_insert)
{
    if (idx >= 0) {
        str->insert(idx, to_insert);
    }
}

void erase(std::string *str, const int32_t first, const int32_t last)
{
    if (first < 0) return;
    if (last == npos) {
        str->erase(first, str->length() - first);
    } else if (last > first) {
        str->erase(first, last-first);
    }
}

void eraseRange(std::string *str, const Range &range)
{
    erase(str, range.begin_, range.end_);
}

int32_t skipFwd(const char *str, const int32_t start, const int32_t numchars)
{
    return(skip_chars_forward(str + start, numchars) - str);
}

int32_t skipBkw(const char *str, const int32_t start, const int32_t numchars)
{
    if (start == npos) {
        return(skip_chars_reverse(str + strlen(str), numchars, str) - str);
    } else {
        return(skip_chars_reverse(str + start, numchars, str) - str);
    }
}

std::string encode(const std::vector<int32_t> &src)
{
    std::string out;
    for (int idx = 0; idx < src.size(); ++idx) {
        cp_encode(&out, src[idx]);
    }
    return(out);
}

void decode(const char *src, std::vector<int32_t> *dst)
{
    dst->clear();
    while (*src != 0) {
        dst->push_back(cp_decode(&src));
    }
}

int32_t decodeOne(const char *src, int32_t *at)
{
    const char *scan = src + *at;
    int32_t value = cp_decode(&scan);
    *at = scan - src;
    return(value);
}

std::string encodeOne(const int32_t src) 
{
    std::string out;
    cp_encode(&out, src);
    return(out);
}

bool isDigit(const int32_t cc)
{
    return(cc >= '0' && cc <= '9');
}

bool isXdigit(const int32_t cc)
{
    return(cc >= '0' && cc <= '9' || cc >= 'a' && cc <= 'f' || cc >= 'A' && cc <= 'F');
}

bool isLetter(const int32_t cc)
{
    return(get_record(cc) != nullptr);
}

bool isUpper(const int32_t cc)
{
    const CaseRange *desc = get_record(cc);
    if (desc == nullptr) return(false);
    int32_t delta =  desc->deltas[0];
    return (delta == 0 || delta == UpperLower && ((cc - desc->low) & 1) == 0);
}

bool isLower(const int32_t cc)
{
    const CaseRange *desc = get_record(cc);
    if (desc == nullptr) return(false);
    int32_t delta =  desc->deltas[1];
    return (delta == 0 || delta == UpperLower && ((cc - desc->low) & 1) != 0);
}

bool isTitle(const int32_t cc)
{
    const CaseRange *desc = get_record(cc);
    if (desc == nullptr) return(false);
    int32_t delta =  desc->deltas[2];
    return (delta == 0 || delta == UpperLower && ((cc - desc->low) & 1) == 0);
}

bool isSpace(const int32_t cc)
{
    switch (cc) {
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
    case 0x85:
    case 0xA0:
    case 0x1680:
    case 0x2028:
    case 0x2029:
    case 0x202f:
    case 0x205f:
    case 0x3000:
        return (true);
    }
    return(cc >= 0x2000 && cc <= 0x200a);
}

int32_t ccToupper(const int32_t cc)
{
    if ((cc & 0x80) == 0) {
        if (cc >= 'a' && cc <= 'z') {
            return(cc + ('A'-'a'));
        }
        return(cc);
    }
    const CaseRange *desc = get_record(cc);
    if (desc == nullptr) return(cc);
    int32_t delta =  desc->deltas[0];
    if (delta == UpperLower) {
        return(desc->low + ((cc - desc->low) & ~1));
    }
    return(cc + delta);
}

int32_t ccTolower(const int32_t cc)
{
    if ((cc & 0x80) == 0) {
        if (cc >= 'A' && cc <= 'Z') {
            return(cc + ('a'-'A'));
        }
        return(cc);
    }
    const CaseRange *desc = get_record(cc);
    if (desc == nullptr) return(cc);
    int32_t delta =  desc->deltas[1];
    if (delta == UpperLower) {
        return(desc->low + ((cc - desc->low) | 1));
    }
    return(cc + delta);
}

int32_t ccTotitle(const int32_t cc)
{
    if ((cc & 0x80) == 0) {
        if (cc >= 'a' && cc <= 'z') {
            return(cc + ('A'-'a'));
        }
        return(cc);
    }
    const CaseRange *desc = get_record(cc);
    if (desc == nullptr) return(cc);
    int32_t delta =  desc->deltas[2];
    if (delta == UpperLower) {
        return(desc->low + ((cc - desc->low) & ~1));
    }
    return(cc + delta);
}

// implementation of local functions

static void cp_encode(std::string *dst, int32_t cp)
{
    if (cp < 0x80) {
        if (cp != 0) {  // the terminator is automatically placed by the std::string class !!
            (*dst) += cp;
        }
    } else if (cp < 0x800) {
        (*dst) += (cp >> 6) | 0xc0;
        (*dst) += (cp & 0x3f) | 0x80;
    } else if (cp < 0x10000) {
        (*dst) += (cp >> 12) | 0xe0;
        (*dst) += ((cp >> 6) & 0x3f) | 0x80;
        (*dst) += (cp & 0x3f) | 0x80;
    } else {
        (*dst) += ((cp >> 18) & 7) | 0xf0;
        (*dst) += ((cp >> 12) & 0x3f) | 0x80;
        (*dst) += ((cp >> 6) & 0x3f) | 0x80;
        (*dst) += (cp & 0x3f) | 0x80;
    }
}

static int32_t cp_decode_impl(const char **src)
{
    int32_t value = -1;

    do {
        value = decode_or_skip(src);
    } while (value == -1);
    return(value);
}

int32_t decode_or_skip(const char **src)
{
    int32_t value = -1;
    const char *scan = *src;

    if ((*scan & 0x80) == 0) {
        value = *scan;
        if (value != 0) ++scan;  // dont' advance past the terminator !!
    } else if ((*scan & 0xe0) == 0xc0) {
        if ((scan[1] & 0xc0) != 0x80) {
            ++scan;
        } else {
            value = ((scan[0] & 0x1f) << 6) | (scan[1] & 0x3f);
            scan += 2;
        }
    } else if ((*scan & 0xf0) == 0xe0) {
        if ((scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80) {
            ++scan;
        } else {
            value = ((scan[0] & 0xf) << 12) | ((scan[1] & 0x3f) << 6) | (scan[2] & 0x3f);
            scan += 3;
        }
    } else if ((*scan & 0xf8) == 0xf0) {
        if ((scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80 || (scan[3] & 0xc0) != 0x80) {
            ++scan;
        } else {
            value = ((scan[0] & 0x7) << 18) | ((scan[1] & 0x3f) << 12) | ((scan[2] & 0x3f) << 6) | (scan[3] & 0x3f);
            scan += 4;
        }
    } else {
        // illegal !, just skip.
        ++scan;
    }
    *src = scan;
    return(value);
}

static bool is_good_cp(const char *src)
{
    return ((*src & 0x80) == 0 || 
            (*src & 0xe0) == 0xc0 && (src[1] & 0xc0) == 0x80 ||
            (*src & 0xf0) == 0xe0 && (src[1] & 0xc0) == 0x80 && (src[2] & 0xc0) == 0x80 ||
            (*src & 0xf8) == 0xf0 && (src[1] & 0xc0) == 0x80 && (src[2] & 0xc0) == 0x80 && (src[3] & 0xc0) == 0x80);
}

static const char *skip_chars_forward(const char *start, int32_t num_chars)
{
    int skipped = 0;

    if (num_chars < 0) return(start);

    // skip num_chars character starts
    while (*start != 0) {
        if (starting(start)) {
            if (skipped == num_chars) return(start);
            ++skipped;            
        }
        ++start;
    }
    return(start);
}

static const char *skip_chars_reverse(const char *start, int32_t num_chars, const char *base)
{
    int skipped = 0;

    if (num_chars < 0 || start < base) return(base);
    
    // skip num_chars character starts
    while (start > base) {
        if (starting(start)) {
            if (skipped == num_chars) return(start);
            ++skipped;
        }
        --start;
    }
    return(base);
}

#ifdef _WIN32

std::string utf16_to_8(const wchar_t *src)
{
    std::string result;
    const wchar_t *scan;

    int len = 0;
    for (scan = src; *scan != 0; ++scan);
    result.reserve((scan - src) * 3 + 1); // worst case

    for (;;) {
        wchar_t value = *src++;
        if (value == 0) {
            return(result);
        } else if (value < 0xd800) {
            cp_encode(&result, value);
        } else if (value < 0xdc00) {
            wchar_t v_low = *src;
            if (v_low >= 0xdc00 && v_low < 0xe000) {
                cp_encode(&result, 0x100000 | ((value - 0xd800) << 10) | (v_low - 0xdc00));
                ++src;
            } else {
                cp_encode(&result, value);
            }
        } else {
            cp_encode(&result, value);
        }
    }
}

void utf8_to_16(const char *src, std::vector<wchar_t> *dst)
{
    int32_t at = 0;
    int32_t cp;

    dst->reserve(dst->size() + strlen(src) + 1);
    do {
        cp = decodeOne(src, &at);
        if (cp < 0x10000) {
            dst->push_back(cp);
        } else {
            dst->push_back(((cp >> 10) & 0x3ff) + 0xd800);
            dst->push_back((cp & 0x3ff) + 0xdc00);
        }
    } while (cp != 0);
}

#endif

}   // namespace
