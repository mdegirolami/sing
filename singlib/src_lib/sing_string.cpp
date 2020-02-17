#include <string.h>
#include <stdexcept>
#include <ctype.h>
#include "sing_string.h"

namespace sing {

void string::rune_encode(char **dst, int32_t rune)
{
    if (rune < 0x80) {
        **dst = rune;
        *dst += 1;
    } else if (rune < 0x800) {
        (*dst)[0] = (rune >> 6) | 0xc0;
        (*dst)[1] = (rune & 0x3f) | 0x80;
        *dst += 2;
    } else if (rune < 0x10000) {
        (*dst)[0] = (rune >> 12) | 0xe0;
        (*dst)[1] = ((rune >> 6) & 0x3f) | 0x80;
        (*dst)[2] = (rune & 0x3f) | 0x80;
        *dst += 3;
    } else {
        (*dst)[0] = ((rune >> 18) & 7) | 0xf0;
        (*dst)[1] = ((rune >> 12) & 0x3f) | 0x80;
        (*dst)[2] = ((rune >> 6) & 0x3f) | 0x80;
        (*dst)[3] = (rune & 0x3f) | 0x80;
        *dst += 4;
    }
}

int32_t string::rune_decode(const char **src)
{
    const char *scan = *src;
    bool        done = false;
    int32_t     value;

    while (!done) {
        if ((*scan & 0x80) == 0) {
            value = *scan;
            if (value != 0) ++scan;  // dont' advance past the terminator !!
            done = true;
        } else if ((*scan & 0xe0) == 0xc0) {
            if ((scan[1] & 0xc0) != 0x80) {
                ++scan;
            } else {
                value = ((scan[0] & 0x1f) << 6) | (scan[1] & 0x3f);
                scan += 2;
                done = true;
            }
        } else if ((*scan & 0xf0) == 0xe0) {
            if ((scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80) {
                ++scan;
            } else {
                value = ((scan[0] & 0xf) << 12) | ((scan[1] & 0x3f) << 6) | (scan[2] & 0x3f);
                scan += 3;
                done = true;
            }
        } else if ((*scan & 0xf8) == 0xf0) {
            if ((scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80 || (scan[3] & 0xc0) != 0x80) {
                ++scan;
            } else {
                value = ((scan[0] & 0x7) << 18) | ((scan[1] & 0x3f) << 12) | ((scan[2] & 0x3f) << 6) | (scan[3] & 0x3f);
                scan += 4;
                done = true;
            }
        } else {
            // illegal !, just skip.
            ++scan;
        }
    }
    *src = scan;
    return(value);
}

const char* string::next_rune_pos(const char *src)
{
    while ((*src & 0xc0) == 0x80 || (*src & 0xf8) == 0xf8) ++src;
    return(src);
}

void string::move(char *src, int delta)
{
    char *dst;
    if (delta < 0) {
        dst = src + delta;
        while (*src) {
            *dst++ = *src++;
        }
    } else if (delta > 0) {
        char *scan = src;
        while (*scan != 0) ++scan;
        dst = scan + delta;
        while (scan >= src) {
            *dst-- = *scan--;
        }
    }
}

int32_t string::length(void) const
{
    int32_t len = 0;
    for (const char *scan = content_; *scan != 0; ++scan) {
        if ((*scan & 0xc0) != 0x80) ++len;
    }
    return(len);
}

void string::operator=(const string &other)
{
    if (allocated_ == 0 && other.allocated_ == 0) {
        content_ = other.content_;
    } else {
        discardAndgrow(other.size());
        strcpy(content_, other.content_);
    }
}

void string::operator=(const char *other)
{
    if (allocated_ == 0) {
        content_ = (char*)other;
    } else {
        discardAndgrow(strlen(other));
        strcpy(content_, other);
    }
}

string& string::operator+=(const string& other)
{
    int len = size();

    growTo(len + other.size());
    strcpy(content_ + len, other.content_);
    return(*this);
}

string& string::operator+=(const char *cstr)
{
    int len = size();

    growTo(len + strlen(cstr));
    strcpy(content_ + len, cstr);
    return(*this);
}

string& string::operator+=(uint32_t rune)
{
    char    *dst;
    int len = size();

    growTo(len + 4);
    dst = content_ + len;
    rune_encode(&dst, rune);
    *dst = 0;
    return(*this);
}

int32_t string::get_rune(int32_t position, int32_t *rune) const
{
    const char *src = content_ + position;
    *rune = rune_decode(&src);
    return(src - content_);
}

int32_t string::overwrite_rune(int32_t position, int32_t rune)
{
    char buffer[4];
    char *insert;
    int  delta, rune_len, old_rune_len;

    // compare the length of the old and new runes
    rune_len = rune_to_buffer(buffer, rune);
    insert = content_ + position;
    old_rune_len = next_rune_pos(insert + 1) - insert;
    delta = rune_len - old_rune_len;

    // grown the buffer if needed/create if a literal
    if (delta > 0) {
        growTo(size() + delta);
    } else {
        growTo(size());             // create if literal
    }

    // may have changed
    insert = content_ + position;

    // make room/compact
    move(insert + old_rune_len, delta);
    
    // actually insert
    for (int idx = 0; idx < rune_len; ++idx) {
        *insert++ = buffer[idx];
    }
    return(position + rune_len);
}

int32_t string::insert_rune(int32_t position, int32_t rune)
{
    char buffer[4];
    char *insert;
    int  rune_len;

    rune_len = rune_to_buffer(buffer, rune);

    // grown the buffer if needed/create if a literal
    growTo(size() + rune_len);

    // make room
    insert = content_ + position;
    move(insert, rune_len);

    // actually insert
    for (int idx = 0; idx < rune_len; ++idx) {
        *insert++ = buffer[idx];
    }
    return(position + rune_len);
}

int32_t string::pos2charidx(int32_t position) const
{
    int32_t pos = 0;
    int32_t idx = 0;
    const char *scan = content_;
    for (int pos = 0; pos < position && *scan != 0; ++pos, ++scan) {
        if ((*scan & 0xc0) != 0x80) ++idx;
    }
    return(idx);
}

int32_t string::charidx2pos(int32_t char_index) const
{
    int32_t pos = 0;
    int32_t idx = 0;
    const char *scan = content_;
    for (pos = 0; idx < char_index && *scan != 0; ++pos, ++scan) {
        if ((*scan & 0xc0) != 0x80) ++idx;
    }
    for (; (*scan & 0xc0) == 0x80; ++pos, ++scan);
    return(pos);
}

string& string::erase_from_pos(int32_t pos, int32_t n)
{
    int32_t len = size();
    int32_t toerase = len - pos;

    if (n != npos && toerase > n) toerase = n;
    if (toerase > 0) {
        growTo(len);              // to allocate if it was a literal
        move(content_ + pos + toerase, -toerase);
    }
    return(*this);
}

string& string::erase_from_idx(int32_t char_idx, int32_t n)
{
    int32_t len = size();
    int32_t pos = charidx2pos(char_idx);
    int32_t toerase = len - pos;

    if (n != npos) {
        const char *scan = content_ + pos;
        while (n && *scan != 0) {
            scan = next_rune_pos(scan + 1);
            --n;
        }
        toerase = scan - (content_ + pos);
    }
    if (toerase > 0) {
        growTo(len);              // to allocate if it was a literal
        move(content_ + pos + toerase, -toerase);
    }
    return(*this);
}

void string::insert_from_pos(int position, const char *toinsert)
{
    int len = strlen(content_);
    int len_toinsert = strlen(toinsert);
    if (len < 1) return;
    if (position < 0) {
        position = 0;
    } else if (position >= len) {
        position = len - 1;
    }
    growTo(len + len_toinsert);
    move(content_ + position, len_toinsert);
    char *dst = content_ + position;
    for (int idx = 0; idx < len_toinsert; ++idx) {
        *dst++ = *toinsert++;
    }
}

int32_t string::find_pos(int32_t from, int32_t rune) const
{
    char        buffer[4];
    int         rune_len, ii, len;
    const char  *scan;

    rune_len = rune_to_buffer(buffer, rune);
    len = strlen(content_);
    if (from < 0) {
        from = 0;
    } else if (from >= len) {
        return(npos);
    }
    for (scan = content_ + from; *scan != 0; ++scan) {
        if (*scan == buffer[0]) {
            for (ii = 1; ii < rune_len; ++ii) {
                if (scan[ii] != buffer[ii]) break;
            }
            if (ii == rune_len) {
                return(scan - content_);
            }
        }
    }
    return(npos);
}

int32_t string::rfind_pos(int32_t from, int32_t rune) const
{
    char        buffer[4];
    int         rune_len, ii, start;
    const char  *scan;

    rune_len = rune_to_buffer(buffer, rune);
    start = strlen(content_) - 1;
    if (from < start && from > 0) start = from;
    for (scan = content_ + start; scan != 0; --scan) {
        if (*scan == buffer[0]) {
            for (ii = 1; ii < rune_len; ++ii) {
                if (scan[ii] != buffer[ii]) break;
            }
            if (ii == rune_len) {
                return(scan - content_);
            }
        }
    }
    return(npos);
}

int32_t string::find_idx(int32_t from, int32_t rune) const
{
    return(pos2charidx(find_pos(charidx2pos(from), rune)));
}

int32_t string::rfind_idx(int32_t from, int32_t rune) const
{
    return(pos2charidx(rfind_pos(charidx2pos(from), rune)));
}

string string::substr_from_pos(int32_t first_pos, int32_t last_pos) const
{
    string  result;
    int     pos, last, count;

    for (pos = 0; pos < first_pos; ++pos) {
        if (content_[pos] == 0) return(result);
    }    
    for (last = pos; last < last_pos && content_[last] != 0; ++last);
    count = last - pos;
    if (count > 0) {
        result.growTo(count);
        char *dst = result.content_;
        for (; pos < last; ++pos) {
            *dst++ = content_[pos];
        }
        *dst++ = 0;
    }
    return(result);
}

string string::substr_from_idx(int32_t first_idx, int32_t last_idx) const
{
    string  result;
    int     pos, last, count, idx;

    pos = 0;
    idx = 0;
    while (true) {
        if (content_[pos] == 0) return(result);
        if ((content_[pos] & 0xc0) != 0x80) ++idx;
        if (idx == first_idx) break;
        ++pos;
    }
    last = pos + 1;
    while (true) {
        if ((content_[pos] & 0xc0) != 0x80) ++idx;
        if (content_[last] == 0 || idx == last_idx) break;
        ++last;
    }
    count = last - pos;
    if (count > 0) {
        result.growTo(count);
        char *dst = result.content_;
        for (; pos < last; ++pos) {
            *dst++ = content_[pos];
        }
        *dst++ = 0;
    }
    return(result);
}

//
// FRIEND FUNCTIONS
//

string operator+(const string& left, const string& right)
{
    string  result;
    int left_len = strlen(left.c_str());

    result.reserve(left_len + strlen(right.c_str()));
    strcpy(result.content_, left.content_);
    strcpy(result.content_ + left_len, right.content_);
    return result;
}

string operator+(uint32_t rune, const string& right)
{
    string  result;
    char    *dst;

    result.growTo(strlen(right.c_str()) + 4);
    dst = result.content_;
    string::rune_encode(&dst, rune);
    strcpy(dst, right.c_str());
    return result;
}

string operator+(const string& left, uint32_t rune)
{
    string  result;
    char    *dst;
    int     left_len = left.size();

    result.growTo(left_len + 4);
    strcpy(result.content_, left.content_);
    dst = result.content_ + left_len;
    string::rune_encode(&dst, rune);
    *dst = 0;
    return result;
}

bool operator==(const string& left, const string& right) { return(strcmp(left.content_, right.content_) == 0); }
bool operator!=(const string& left, const string& right) { return(strcmp(left.content_, right.content_) != 0); }
bool operator<(const string& left, const string& right)  { return(strcmp(left.content_, right.content_) < 0); }
bool operator<=(const string& left, const string& right) { return(strcmp(left.content_, right.content_) <= 0); }
bool operator>(const string& left, const string& right)  { return(strcmp(left.content_, right.content_) > 0); }
bool operator>=(const string& left, const string& right) { return(strcmp(left.content_, right.content_) >= 0); }

/*
void string::utf8_decode(vector<int32_t> *dst) const
{
    int         chars_left;
    const char  *scan;

    dst->reserve(dst->size() + content_.size() - 1);
    chars_left = content_.size();
    scan = &content_[0];
    while (chars_left > 0) {
        if ((*scan & 0x80) == 0) {
            dst->push_back(*scan++);
            --chars_left;
        } else if ((*scan & 0xe0) == 0xc0) {
            if (chars_left < 2 || (scan[1] & 0xc0) != 0x80) {
                ++scan;
                --chars_left;
            } else {
                dst->push_back(((scan[0] & 0x1f) << 6) | (scan[1] & 0x3f));
                scan += 2;
                chars_left -= 2;
            }
        } else if ((*scan & 0xf0) == 0xe0) {
            if (chars_left < 3 || (scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80) {
                ++scan;
                --chars_left;
            } else {
                dst->push_back(((scan[0] & 0xf) << 12) | ((scan[1] & 0x3f) << 6) | (scan[2] & 0x3f));
                scan += 3;
                chars_left -= 3;
            }
        } else if ((*scan & 0xf8) == 0xf0) {
            if (chars_left < 4 || (scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80 || (scan[3] & 0xc0) != 0x80) {
                ++scan;
                --chars_left;
            } else {
                dst->push_back(((scan[0] & 0x7) << 18) | ((scan[1] & 0x3f) << 12) | ((scan[2] & 0x3f) << 6) | (scan[3] & 0x3f));
                scan += 4;
                chars_left -= 4;
            }
        } else {
            // illegal !, just skip.
            ++scan;
            --chars_left;
        }
    }
}

void string::utf8_encode(const int32_t *codepoints, int len)
{
    int     ii;
    int32_t cp;

    // take the terminator out of the way
    content_.pop_back();    

    // append the codepoints
    for (ii = 0; ii < len; ++ii) {
        cp = codepoints[ii];
        if (cp < 0x80) {
            content_.push_back(cp);
        } else if (cp < 0x800) {
            content_.push_back((cp >> 6) | 0xc0);
            content_.push_back((cp & 0x3f) | 0x80);
        } else if (cp < 0x10000) {
            content_.push_back((cp >> 12) | 0xe0);
            content_.push_back(((cp >> 6) & 0x3f) | 0x80);
            content_.push_back((cp & 0x3f) | 0x80);
        } else {
            content_.push_back(((cp >> 18) & 7) | 0xf0);
            content_.push_back(((cp >> 12) & 0x3f) | 0x80);
            content_.push_back(((cp >> 6) & 0x3f) | 0x80);
            content_.push_back((cp & 0x3f) | 0x80);
        }
    }

    // restore the terminator
    if (content_[content_.size() - 1] != 0) {
        content_.push_back(0);
    }
}
*/
}
