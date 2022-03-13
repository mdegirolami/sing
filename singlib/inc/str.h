#pragma once

#include <sing.h>

namespace sing {

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

enum class SplitMode {sm_separator_left, sm_separator_right, sm_drop};

static const int32_t npos = -1;

class Selector {
public:
    virtual ~Selector() {}
    virtual void *get__id() const = 0;
    virtual bool isGood(int32_t cp) const = 0;
};

class Range final {
public:
    Range();
    int32_t begin_;
    int32_t end_;
};

// getting info
int32_t len(const char *src);
int32_t numchars(const char *src);
int32_t count(const char *src, const char *to_search, bool insensitive = false);

// casing
std::string toupper(const char *src);
std::string tolower(const char *src);
std::string totitle(const char *src);
int32_t compare(const char *first, const char *second, bool insensitive = false);
int32_t compareAt(const char *first, int32_t at_pos, const char *contained, int32_t *end_pos, bool insensitive = false);

// splitting
bool split(const char *src, const char *splitter, std::string *left, std::string *right, SplitMode mode = SplitMode::sm_drop, bool insensitive = false);
bool splitAny(const char *src, const char *splitter, std::string *left, std::string *right, SplitMode mode = SplitMode::sm_drop, bool insensitive = false);
bool rsplit(const char *src, const char *splitter, std::string *left, std::string *right, SplitMode mode = SplitMode::sm_drop, bool insensitive = false);
bool rsplitAny(const char *src, const char *splitter, std::string *left, std::string *right, SplitMode mode = SplitMode::sm_drop, bool insensitive = false);

// replacing
int32_t replace(std::string *src, const char *old_sub, const char *new_sub, bool insensitive = false, int32_t from = 0);
int32_t replaceAll(std::string *src, const char *old_sub, const char *new_sub, bool insensitive = false);

// prefix/suffix
bool hasPrefix(const char *src, const char *prefix, bool insensitive = false);
bool hasSuffix(const char *src, const char *suffix, bool insensitive = false);
void cutPrefix(std::string *str, const char *prefix, bool insensitive = false);
void cutSuffix(std::string *str, const char *suffix, bool insensitive = false);

// cleanup
void cutLeadingSpaces(std::string *str);
void cutTrailingSpaces(std::string *str);
void cutLeading(std::string *str, const Selector &to_keep);
void cutTrailing(std::string *str, const Selector &to_keep);
void cutFun(std::string *str, const Selector &to_keep);
void makeUtf8Compliant(std::string *str);

// working with indices
bool find(const char *src, const char *to_search, Range *range, bool insensitive = false, int32_t from = 0);
bool rfind(const char *src, const char *to_search, Range *range, bool insensitive = false, int32_t from = npos);
bool findAny(const char *src, const char *to_search, Range *range, bool insensitive = false, int32_t from = 0);
bool rfindAny(const char *src, const char *to_search, Range *range, bool insensitive = false, int32_t from = npos);
bool findFnc(const char *src, const Selector &matches, Range *range, int32_t from = 0);
bool rfindFnc(const char *src, const Selector &matches, Range *range, int32_t from = npos);

std::string sub(const char *src, int32_t first, int32_t last = npos);
std::string subRange(const char *src, const Range &range);
int32_t pos2idx(const char *src, int32_t pos);
int32_t idx2pos(const char *src, int32_t idx);
void insert(std::string *str, int32_t idx, const char *to_insert);
void erase(std::string *str, int32_t first, int32_t last = npos);
void eraseRange(std::string *str, const Range &range);
int32_t skipFwd(const char *str, int32_t start, int32_t numchars);
int32_t skipBkw(const char *str, int32_t start, int32_t numchars);

// unicode
std::string encode(const std::vector<int32_t> &src);
void decode(const char *src, std::vector<int32_t> *dst);
int32_t decodeOne(const char *src, int32_t *at);
std::string encodeOne(int32_t src);

// char classify
bool isDigit(int32_t cc);
bool isXdigit(int32_t cc);
bool isLetter(int32_t cc);
bool isUpper(int32_t cc);
bool isLower(int32_t cc);
bool isTitle(int32_t cc);
bool isSpace(int32_t cc);

// char case conversion
int32_t ccToupper(int32_t cc);
int32_t ccTolower(int32_t cc);
int32_t ccTotitle(int32_t cc);

}   // namespace
