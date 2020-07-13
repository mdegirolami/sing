#pragma once

#include <sing.h>

namespace sing {

enum class SplitMode {sm_separator_left, sm_separator_right, sm_drop};
static const int32_t npos = -1;
class Selector {
public:
    virtual ~Selector() {}
    virtual void *get__id() const = 0;
    virtual bool is_good(const int32_t cp) const = 0;
};
class Range final {
public:
    Range();
    int32_t begin_;
    int32_t end_;
};

int32_t len(const char *src);
int32_t numchars(const char *src);
int32_t count(const char *src, const char *to_search, const bool insensitive = false);
std::string toupper(const char *src);
std::string tolower(const char *src);
std::string totitle(const char *src);
int32_t compare(const char *first, const char *second, const bool insensitive = false);
int32_t compare_at(const char *first, const int32_t at_pos, const char *contained, int32_t *end_pos, const bool insensitive = false);
bool split(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
bool split_any(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
bool rsplit(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
bool rsplit_any(const char *src, const char *splitter, std::string *left, std::string *right, const SplitMode &mode = SplitMode::sm_drop,
    const bool insensitive = false);
int32_t replace(std::string *src, const char *old_sub, const char *new_sub, const bool insensitive = false, const int32_t from = 0);
int32_t replace_all(std::string *src, const char *old_sub, const char *new_sub, const bool insensitive = false);
bool has_prefix(const char *src, const char *prefix, const bool insensitive = false);
bool has_suffix(const char *src, const char *suffix, const bool insensitive = false);
void cut_prefix(std::string *str, const char *prefix, const bool insensitive = false);
void cut_suffix(std::string *str, const char *suffix, const bool insensitive = false);
void cut_leading_spaces(std::string *str);
void cut_trailing_spaces(std::string *str);
void cut_leading(std::string *str, const Selector &to_keep);
void cut_trailing(std::string *str, const Selector &to_keep);
void cut_fun(std::string *str, const Selector &to_keep);
void make_utf8_compliant(std::string *str);
bool find(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = 0);
bool rfind(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = npos);
bool find_any(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = 0);
bool rfind_any(const char *src, const char *to_search, Range *range, const bool insensitive = false, const int32_t from = npos);
bool find_fnc(const char *src, const Selector &matches, Range *range, const int32_t from = 0);
bool rfind_fnc(const char *src, const Selector &matches, Range *range, const int32_t from = npos);
std::string sub(const char *src, const int32_t first, const int32_t last = npos);
std::string sub_range(const char *src, const Range &range);
int32_t pos2idx(const char *src, const int32_t pos);
int32_t idx2pos(const char *src, const int32_t idx);
void insert(std::string *str, const int32_t idx, const char *to_insert);
void erase(std::string *str, const int32_t first, const int32_t last = npos);
void erase_range(std::string *str, const Range &range);
int32_t skip_fwd(const char *str, const int32_t start, const int32_t numchars);
int32_t skip_bkw(const char *str, const int32_t start, const int32_t numchars);
std::string encode(const std::vector<int32_t> &src);
void decode(const char *src, std::vector<int32_t> *dst);
int32_t decode_one(const char *src, int32_t *at);
std::string encode_one(const int32_t src);
bool is_digit(const int32_t cc);
bool is_xdigit(const int32_t cc);
bool is_letter(const int32_t cc);
bool is_upper(const int32_t cc);
bool is_lower(const int32_t cc);
bool is_title(const int32_t cc);
bool is_space(const int32_t cc);
int32_t cc_toupper(const int32_t cc);
int32_t cc_tolower(const int32_t cc);
int32_t cc_totitle(const int32_t cc);

}   // namespace
