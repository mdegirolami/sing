#pragma once

#include <sing.h>

namespace sing {

enum class TokenType {string_type, number, symbol, interpunction, comment, eol, eof};

class Lexer final {
public:
    Lexer();
    void init(const char *json);
    bool stepToNextToken();

    std::string getErrorString() const;
    void getCurrentPosition(int32_t *row, int32_t *column);
    void resetError();

    TokenType getTokenType() const;
    std::string getTokenString() const;
    bool isTokenSeparated() const;

private:

    bool parseNumber(int32_t cc);
    bool parseSymbol(int32_t cc);
    bool parseString();
    bool parseComment();
    void skipLine();

    std::vector<int32_t> codes_;
    int32_t scan_;

    int32_t curline_;
    int32_t line_begin_;

    TokenType token_;
    bool separated_;
    std::string error_;
    std::string string_rep_;

    // aux stuff
    std::vector<int32_t> buffer_;
};
enum class JsonEntryType {number, string_type, boolean, null_type, array, object, comment};

class JsonEntry final {
public:
    JsonEntry();
    JsonEntryType entry_type_;
    int32_t level_;
    std::string string_rep_;
    std::string property_name_;
};

class JsonError final {
public:
    JsonError();
    void setString(const char *val);
    void setPos(int32_t row, int32_t col);

    std::string getString() const;
    void getLocation(int32_t *row, int32_t *col) const;

private:
    std::string err_;
    int32_t row_;
    int32_t col_;
};

bool parseJson(const char *json, std::vector<JsonEntry> *records, std::vector<JsonError> *errors);
bool writeJson(const std::vector<JsonEntry> &records, std::string *json);

}   // namespace
