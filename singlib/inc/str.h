#pragma once

#include <sing.h>

namespace sing {

enum class SplitMode {sm_separator_left, sm_separator_right, sm_drop};

static const int32_t npos = -1;

class Selector {
public:
    virtual ~Selector() {}
    virtual void *get__id() const = 0;
    virtual bool isGood(const int32_t cp) const = 0;
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
int32_t count(const char *src, const char *to_search, const bool insensitive = false);

// casing
std::string toupper(const char *src);
std::string tolower(const char *src);
std::string totitle(const char *src);
int32_t compare(const char *first, const char *second, const bool insensitive = false);
int32_t compareAt(const char *first, const int32_t at_pos, const char *contained, int32_t *end_pos, const bool insensitive = false);

// splitting
bool split(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
bool split_any(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
bool rsplit(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
bool rsplitAny(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);

// replacing
int32_t replace(std::string *src, const char *old_sub, const char *new_sub, const bool insensitive = false, const int32_t from = 0);
int32_t replaceAll(std::string *src, const char *old_sub, const char *new_sub, const bool insensitive = false);

// prefix/suffix
bool hasPrefix(const char *src, const char *prefix, const bool insensitive = false);
bool hasSuffix(const char *src, const char *suffix, const bool insensitive = false);
void cutPrefix(std::string *str, const char *prefix, const bool insensitive = false);
void cutSuffix(std::string *str, const char *suffix, const bool insensitive = false);

// cleanup
void cutLeadingSpaces(std::string *str);
void cutTrailingSpaces(std::string *str);
void cutLeading(std::string *str, const Selector &to_keep);
void cutTrailing(std::string *str, const Selector &to_keep);
void cutFun(std::string *str, const Selector &to_keep);
void makeUtf8Compliant(std::string *str);

// working with indices
bool find(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = 0);
bool rfind(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = npos);
bool findAny(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = 0);
bool rfindAny(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = npos);
bool findFnc(const char *src, const Selector &matches, Range *range, const int32_t from = 0);
bool rfindFnc(const char *src, const Selector &matches, Range *range, const int32_t from = npos);

std::string sub(const char *src, const int32_t first, const int32_t last = npos);
std::string sub_range(const char *src, const Range &range);
int32_t pos2idx(const char *src, const int32_t pos);
int32_t idx2pos(const char *src, const int32_t idx);
void insert(std::string *str, const int32_t idx, const char *to_insert);
void erase(std::string *str, const int32_t first, const int32_t last = npos);
void erase_range(std::string *str, const Range &range);
int32_t skipFwd(const char *str, const int32_t start, const int32_t numchars);
int32_t skipBkw(const char *str, const int32_t start, const int32_t numchars);

// unicode
std::string encode(const std::vector<int32_t> &src);
void decode(const char *src, std::vector<int32_t> *dst);
int32_t decodeOne(const char *src, int32_t *at);
std::string encodeOne(const int32_t src);

// char classify
bool isDigit(const int32_t cc);
bool isXdigit(const int32_t cc);
bool isLetter(const int32_t cc);
bool isUpper(const int32_t cc);
bool isLower(const int32_t cc);
bool isTitle(const int32_t cc);
bool isSpace(const int32_t cc);

// char case conversion
int32_t ccToupper(const int32_t cc);
int32_t ccTolower(const int32_t cc);
int32_t ccTotitle(const int32_t cc);

}   // namespace
