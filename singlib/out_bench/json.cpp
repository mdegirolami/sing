#include "json.h"
#include "str.h"
#include "sio.h"

namespace sing {

static const int32_t doubleQuoteAscii = 0x22;
static const int32_t plusAscii = 0x2b;
static const int32_t minusAscii = 0x2d;
static const int32_t dotAscii = 0x2e;
static const int32_t slashAscii = 0x2f;
static const int32_t zeroAscii = 0x30;
static const int32_t backslashAscii = 0x5c;
static const int32_t bAscii = 0x62;
static const int32_t eAscii = 0x65;
static const int32_t fAscii = 0x66;
static const int32_t nAscii = 0x6e;
static const int32_t rAscii = 0x72;
static const int32_t tAscii = 0x74;
static const int32_t uAscii = 0x75;

enum class NumState {int_first, int_part, fract_first, fract_part, exp_sign, exp_first, exp, done};

enum class StrState {std, escape, xdigits, done};

static bool parseValues(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors);
static bool parseProperties(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors);
static bool parseSingleValue(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors);
static bool lexerAdvance(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors);
static void insertRecord(JsonEntryType record_type, int32_t level, const char *string_rep, std::vector<JsonEntry> *records);
static void setError(const char *message, Lexer *lexer, std::vector<JsonError> *errors);
static std::string addEscapesAndQuotes(const char *to_fix);

static const std::string tab_str = "    ";

JsonEntry::JsonEntry()
{
    entry_type_ = JsonEntryType::number;
    level_ = 0;
}

JsonError::JsonError()
{
    row_ = 0;
    col_ = 0;
}

void JsonError::setString(const char *val)
{
    err_ = val;
}

void JsonError::setPos(int32_t row, int32_t col)
{
    this->row_ = row;
    this->col_ = col;
}

std::string JsonError::getString() const
{
    return (err_);
}

void JsonError::getLocation(int32_t *row, int32_t *col) const
{
    *row = this->row_;
    *col = this->col_;
}

Lexer::Lexer()
{
    scan_ = 0;
    curline_ = 0;
    line_begin_ = 0;
    token_ = TokenType::string_type;
    separated_ = false;
}

void Lexer::init(const char *json)
{
    codes_.reserve(1000);
    buffer_.reserve(100);
    decode(json, &codes_);
    scan_ = 0;
    separated_ = true;
    curline_ = 1;
    line_begin_ = 0;
}

bool Lexer::stepToNextToken()
{
    token_ = TokenType::eof;
    separated_ = false;
    while (scan_ < codes_.size()) {
        const int32_t cc = codes_[scan_];
        ++scan_;
        if (cc == minusAscii || isDigit(cc)) {
            return (parseNumber(cc));
        } else if (isLetter(cc)) {
            return (parseSymbol(cc));
        } else if (cc == doubleQuoteAscii) {
            return (parseString());
        } else if (cc == slashAscii) {
            return (parseComment());
        } else if (cc > 32 && cc < 127) {
            token_ = TokenType::interpunction;
            string_rep_ = encodeOne(cc);
            return (true);
        } else if (cc == 10) {
            token_ = TokenType::eol;
            return (true);
        } else if (isSpace(cc) || cc == 13) {
            separated_ = true;
        } else {
            error_ = "unexpected character";
            return (false);
        }
    }
    return (true);
}

bool Lexer::parseNumber(int32_t cc)
{
    NumState status = NumState::int_part;
    buffer_.clear();
    buffer_.push_back(cc);
    if (cc == minusAscii) {
        status = NumState::int_first;
    }
    while (scan_ < codes_.size() && status != NumState::done) {
        const int32_t nc = codes_[scan_];
        ++scan_;

        // always right if not finished and not in error.
        buffer_.push_back(nc);

        switch (status) {
        case NumState::int_first: 
            {
                if (!isDigit(nc)) {
                    error_ = "expecting a digit";
                    return (false);
                }
                status = NumState::int_part;
            }
            break;
        case NumState::int_part: 
            {
                if (nc == dotAscii) {
                    status = NumState::fract_first;
                } else if ((nc | 0x20) == eAscii) {
                    status = NumState::exp_sign;
                } else if (!isDigit(nc)) {
                    status = NumState::done;
                }
            }
            break;
        case NumState::fract_first: 
            {
                if (isDigit(nc)) {
                    status = NumState::fract_part;
                } else {
                    error_ = "the first digit after the point must be a digit";
                    return (false);
                }
            }
            break;
        case NumState::fract_part: 
            {
                if ((nc | 0x20) == eAscii) {
                    status = NumState::exp_sign;
                } else if (!isDigit(nc)) {
                    status = NumState::done;
                }
            }
            break;
        case NumState::exp_sign: 
            {
                if (isDigit(nc)) {
                    status = NumState::exp;
                } else if (nc == plusAscii) {
                    status = NumState::exp_first;
                } else if (nc == minusAscii) {
                    status = NumState::exp_first;
                } else {
                    error_ = "expecting the first exponent digit";
                    return (false);
                }
            }
            break;
        case NumState::exp_first: 
            {
                if (isDigit(nc)) {
                    status = NumState::exp;
                } else {
                    error_ = "expecting the first exponent digit";
                    return (false);
                }
            }
            break;
        case NumState::exp: 
            {
                if (!isDigit(nc)) {
                    status = NumState::done;
                }
            }
            break;
        case NumState::done: 
            {
            }
            break;
        }
    }

    // eof ?
    if (status != NumState::done) {
        error_ = "truncated number";
        return (false);
    }

    // pop terminator
    buffer_.pop_back();
    --scan_;

    // convert to string
    token_ = TokenType::number;
    buffer_.push_back(0);
    string_rep_ = encode(buffer_);

    return (true);
}

bool Lexer::parseSymbol(int32_t cc)
{
    buffer_.clear();
    buffer_.push_back(cc);
    while (scan_ < codes_.size()) {
        const int32_t nc = codes_[scan_];
        if (isDigit(nc) || isLetter(nc)) {
            buffer_.push_back(nc);
            ++scan_;
        } else {
            break;
        }
    }
    token_ = TokenType::symbol;
    buffer_.push_back(0);
    string_rep_ = encode(buffer_);
    return (true);
}

bool Lexer::parseString()
{
    std::vector<int32_t> xdigits;
    StrState state = StrState::std;
    buffer_.clear();
    while (scan_ < codes_.size() && state != StrState::done) {
        const int32_t nc = codes_[scan_];
        ++scan_;

        switch (state) {
        case StrState::std: 
            {
                if (nc == doubleQuoteAscii) {
                    state = StrState::done;
                } else if (nc == backslashAscii) {
                    state = StrState::escape;
                } else if (nc < 0x20) {
                    error_ = "control chars are unallowed in strings (pls use escape sequencies)";
                    return (false);
                } else {
                    buffer_.push_back(nc);
                }
            }
            break;
        case StrState::escape: 
            {
                state = StrState::std;
                switch (nc) {
                case backslashAscii: 
                case doubleQuoteAscii: 
                    buffer_.push_back(nc);
                    break;
                case bAscii: 
                    buffer_.push_back(8);
                    break;
                case fAscii: 
                    buffer_.push_back(12);
                    break;
                case nAscii: 
                    buffer_.push_back(10);
                    break;
                case rAscii: 
                    buffer_.push_back(13);
                    break;
                case tAscii: 
                    buffer_.push_back(9);
                    break;
                case uAscii: 
                    {
                        state = StrState::xdigits;
                        xdigits.clear();
                        xdigits.reserve(5);
                    }
                    break;
                default:
                    {
                        error_ = "wrong escape sequence";
                        return (false);
                    }
                    break;
                }
            }
            break;
        case StrState::xdigits: 
            {
                if (isXdigit(nc)) {
                    xdigits.push_back(nc);
                    if (xdigits.size() == 4) {
                        xdigits.push_back(0);

                        uint64_t value = 0;
                        int32_t last_pos = 0;
                        parseUnsignedHex(&value, encode(xdigits).c_str(), 0, &last_pos);
                        buffer_.push_back((int32_t)value);
                        state = StrState::std;
                    }
                } else {
                    error_ = "wrong escape sequence";
                    return (false);
                }
            }
            break;
        case StrState::done: 
            {
            }
            break;
        }
    }
    token_ = TokenType::string_type;
    buffer_.push_back(0);
    string_rep_ = encode(buffer_);
    return (true);
}

bool Lexer::parseComment()
{
    if (scan_ >= codes_.size() || codes_[scan_] != slashAscii) {
        error_ = "expecting /";
        return (false);
    }
    ++scan_;

    buffer_.clear();
    buffer_.push_back(slashAscii);
    buffer_.push_back(slashAscii);
    while (scan_ < codes_.size() && codes_[scan_] != 10) {
        if (codes_[scan_] > 31) {
            buffer_.push_back(codes_[scan_]);
        }
        ++scan_;
    }
    token_ = TokenType::comment;
    buffer_.push_back(0);
    string_rep_ = encode(buffer_);
    return (true);
}

void Lexer::skipLine()
{
    while (scan_ < codes_.size() && codes_[scan_] != 10) {
        ++scan_;
    }
    if (scan_ < codes_.size()) {
        token_ = TokenType::eol;
        ++scan_;
    } else {
        token_ = TokenType::eof;
    }
}

std::string Lexer::getErrorString() const
{
    return (error_);
}

void Lexer::getCurrentPosition(int32_t *row, int32_t *column)
{
    *column = 1;
    for(int32_t idx = line_begin_, idx__top = scan_; idx < idx__top; ++idx) {
        const int32_t cc = codes_[idx];
        if (cc == 10) {
            line_begin_ = idx + 1;
            ++curline_;
            *column = 1;
        } else if (cc == 9) {
            *column += 2;
        } else if (cc >= 32) {
            *column += 1;
        }
    }
    *row = curline_;
}

void Lexer::resetError()
{
    skipLine();
}

TokenType Lexer::getTokenType() const
{
    return (token_);
}

std::string Lexer::getTokenString() const
{
    return (string_rep_);
}

bool Lexer::isTokenSeparated() const
{
    return (separated_);
}

// here on: the parser
bool parseJson(const char *json, std::vector<JsonEntry> *records, std::vector<JsonError> *errors)
{
    Lexer lexer;

    lexer.init(json);
    if (!lexerAdvance(0, &lexer, &*records, &*errors)) {
        return (false);
    }
    return (parseValues(0, &lexer, &*records, &*errors));
}

// parses 0 or more comma separated values. the list can end with an extra comma.
static bool parseValues(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors)
{
    int32_t recsize = (*records).size();
    while (parseSingleValue(level, &*lexer, &*records, &*errors)) {

        // no more values (what is next doesn't match the value syntax)
        if (recsize == (*records).size()) {
            return (true);
        }
        recsize = (*records).size();

        if ((*lexer).getTokenType() != TokenType::interpunction || (*lexer).getTokenString() != ",") {
            return (true);
        }

        // there is a comma: try again
        if (!lexerAdvance(level, &*lexer, &*records, &*errors)) {
            return (false);
        }
    }
    return (false);
}

// parses 0 or more comma separated properties. the list can end with an extra comma.
static bool parseProperties(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors)
{
    int32_t recsize = (*records).size();
    while ((*lexer).getTokenType() == TokenType::string_type) {

        // absorb property name
        const std::string property_name = (*lexer).getTokenString();
        if (!lexerAdvance(level, &*lexer, &*records, &*errors)) {
            return (false);
        }

        // need a ':'
        if ((*lexer).getTokenType() != TokenType::interpunction || (*lexer).getTokenString() != ":") {
            setError("expecting a ':'", &*lexer, &*errors);
            return (false);
        }
        if (!lexerAdvance(level, &*lexer, &*records, &*errors)) {
            return (false);
        }

        // read the value
        if (!parseSingleValue(level, &*lexer, &*records, &*errors)) {
            return (false);
        }

        // if we got one (we must), fix the property name
        if (recsize == (*records).size()) {
            setError("expecting a value", &*lexer, &*errors);
            return (false);
        } else {
            (*records)[recsize].property_name_ = property_name;
            recsize = (*records).size();
        }

        // if a comma is not following we are done
        if ((*lexer).getTokenType() != TokenType::interpunction || (*lexer).getTokenString() != ",") {
            return (true);
        }

        // there is a comma: try again
        if (!lexerAdvance(level, &*lexer, &*records, &*errors)) {
            return (false);
        }
    }

    // what follow is not another prop.
    return (true);
}

// returns a value, if any - you can check if a value was actually found testing the records length
static bool parseSingleValue(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors)
{
    // if an error occurs, return (false) from the switch
    // if there is not a value, return (true) from the switch 
    switch ((*lexer).getTokenType()) {
    case TokenType::number: 
        {
            insertRecord(JsonEntryType::number, level, (*lexer).getTokenString().c_str(), &*records);
        }
        break;
    case TokenType::string_type: 
        {
            insertRecord(JsonEntryType::string_type, level, (*lexer).getTokenString().c_str(), &*records);
        }
        break;
    case TokenType::symbol: 
        {
            const std::string symbol = (*lexer).getTokenString();
            if (symbol == "null") {
                insertRecord(JsonEntryType::null_type, level, symbol.c_str(), &*records);
            } else if (symbol == "true" || symbol == "false") {
                insertRecord(JsonEntryType::boolean, level, symbol.c_str(), &*records);
            } else {
                setError("unknown value", &*lexer, &*errors);
                return (false);
            }
        }
        break;
    case TokenType::interpunction: 
        {
            const std::string symbol = (*lexer).getTokenString();
            if (symbol == "[") {
                insertRecord(JsonEntryType::array, level, symbol.c_str(), &*records);
                if (!lexerAdvance(level + 1, &*lexer, &*records, &*errors)) {
                    return (false);
                }
                if (!parseValues(level + 1, &*lexer, &*records, &*errors)) {
                    return (false);
                }
                if ((*lexer).getTokenType() != TokenType::interpunction || (*lexer).getTokenString() != "]") {
                    setError("expecting ']'", &*lexer, &*errors);
                    return (false);
                }
            } else if (symbol == "{") {
                insertRecord(JsonEntryType::object, level, symbol.c_str(), &*records);
                if (!lexerAdvance(level + 1, &*lexer, &*records, &*errors)) {
                    return (false);
                }
                if (!parseProperties(level + 1, &*lexer, &*records, &*errors)) {
                    return (false);
                }
                if ((*lexer).getTokenType() != TokenType::interpunction || (*lexer).getTokenString() != "}") {
                    setError("expecting '}'", &*lexer, &*errors);
                    return (false);
                }
            } else {
                return (true);
            }
        }
        break;
    default:
        {
            return (true);
        }
        break;
    }

    // we got a value !
    // consume the value token or ] }
    return (lexerAdvance(level, &*lexer, &*records, &*errors));
}

static bool lexerAdvance(int32_t level, Lexer *lexer, std::vector<JsonEntry> *records, std::vector<JsonError> *errors)
{
    while (true) {
        if (!(*lexer).stepToNextToken()) {
            setError((*lexer).getErrorString().c_str(), &*lexer, &*errors);
            return (false);
        } else if ((*lexer).getTokenType() == TokenType::comment) {
            insertRecord(JsonEntryType::comment, level, (*lexer).getTokenString().c_str(), &*records);
        } else if ((*lexer).getTokenType() != TokenType::eol) {
            return (true);
        }
    }
}

static void insertRecord(JsonEntryType record_type, int32_t level, const char *string_rep, std::vector<JsonEntry> *records)
{
    const int32_t top = (*records).size();
    (*records).resize(top + 1);
    (*records)[top].entry_type_ = record_type;
    (*records)[top].level_ = level;
    (*records)[top].string_rep_ = string_rep;
}

static void setError(const char *message, Lexer *lexer, std::vector<JsonError> *errors)
{
    const int32_t top = (*errors).size();
    (*errors).resize(top + 1);
    int32_t row = 0;
    int32_t col = 0;
    (*errors)[top].setString(message);
    (*lexer).getCurrentPosition(&row, &col);
    (*errors)[top].setPos(row, col);
}

// only fails if JsonEntry levels sequence is wrong
bool writeJson(const std::vector<JsonEntry> &records, std::string *json)
{
    *json = "";
    std::vector<bool> is_member;
    is_member.reserve(50);
    is_member.push_back(false);         // at level 0 records are not members of object
    int64_t count = -1;
    for(auto &record : records) {
        ++count;

        // if deeper than expecting (without an array/object beginning) is an error
        int32_t curr_level = is_member.size() - 1;
        if (record.level_ > curr_level) {
            return (false);
        }

        // if less deep than expected, one or more obj/array are completed,
        // close them !
        while (curr_level > record.level_) {
            for(int32_t idx = 0, idx__top = curr_level - 1; idx < idx__top; ++idx) {
                *json += tab_str;
            }
            if (is_member[curr_level]) {
                *json += "}";
            } else {
                *json += "]";
            }
            is_member.pop_back();
            --curr_level;
            if (record.level_ == curr_level) {
                *json += ",";
            }
            *json += "\r\n";
        }

        // indent based on level
        for(int32_t idx = 0, idx__top = record.level_; idx < idx__top; ++idx) {
            *json += tab_str;
        }

        // if is a property, write the key
        if (is_member[curr_level] && record.entry_type_ != JsonEntryType::comment) {
            *json += addEscapesAndQuotes(record.property_name_.c_str()) + ": ";
        }

        // write the record
        bool may_need_comma = false;
        switch (record.entry_type_) {
        case JsonEntryType::array: 
            {
                *json += "[";
                is_member.push_back(false);
            }
            break;
        case JsonEntryType::object: 
            {
                *json += "{";
                is_member.push_back(true);
            }
            break;
        case JsonEntryType::string_type: 
            {
                *json += addEscapesAndQuotes(record.string_rep_.c_str());
                may_need_comma = true;
            }
            break;
        case JsonEntryType::comment: 
            {
                *json += record.string_rep_;
            }
            break;
        default:
            {
                *json += record.string_rep_;
                may_need_comma = true;
            }
            break;
        }

        // we must terminate the line with ',' if we are at the same level of the next record
        if (may_need_comma) {

            // search the next record which is not a comment
            int32_t level = -1;
            for(int32_t scan = (int32_t)count + 1, scan__top = records.size(); scan < scan__top; ++scan) {
                if (records[scan].entry_type_ != JsonEntryType::comment) {
                    level = records[scan].level_;
                    break;
                }
            }
            if (level == record.level_) {
                *json += ",";
            }
        }
        *json += "\r\n";
    }

    // close all the open objects/arrays
    for(int32_t curr_level = is_member.size() - 1; curr_level > 0; --curr_level) {
        for(int32_t idx = 0, idx__top = curr_level - 1; idx < idx__top; ++idx) {
            *json += tab_str;
        }
        if (is_member[curr_level]) {
            *json += "}\r\n";
        } else {
            *json += "]\r\n";
        }
    }
    return (true);
}

static std::string addEscapesAndQuotes(const char *to_fix)
{
    std::vector<int32_t> codes;
    std::string fixed = "\"";
    codes.reserve(128);
    decode(to_fix, &codes);
    for(auto &code : codes) {
        switch (code) {
        case 8: 
            fixed += "\\b";
            break;
        case 9: 
            fixed += "\\t";
            break;
        case 10: 
            fixed += "\\n";
            break;
        case 12: 
            fixed += "\\f";
            break;
        case 13: 
            fixed += "\\r";
            break;
        case doubleQuoteAscii: 
            fixed += "\\\"";
            break;
        case backslashAscii: 
            fixed += "\\\\";
            break;
        default:
            if (code < 32 || code > 126) {
                fixed += "\\u" + formatUnsignedHex((uint64_t)code, 4, f_zero_prefix);
            } else {
                fixed += encodeOne(code);
            }
            break;
        }
    }
    fixed += "\"";
    return (fixed);
}

}   // namespace
