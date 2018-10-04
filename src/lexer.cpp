#include <assert.h>
#include <wctype.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "lexer.h"
#include "helpers.h"

namespace SingNames {

struct TokenDesc {
    Token       token;
    const char *token_string;
};

TokenDesc keywords[] = {
    {TOKEN_COMMENT, "/*"},
    {TOKEN_INLINE_COMMENT, "//"},
    {TOKEN_LITERAL_STRING, "\""},

    {TOKEN_NULL, "null"},
    {TOKEN_TRUE, "true"},
    {TOKEN_FALSE, "false"},
    {TOKEN_VOID, "void"},

    {TOKEN_PACKAGE, "package"},
    {TOKEN_REQUIRES, "requires"},
    {TOKEN_VAR, "var"},
    {TOKEN_CONST, "const"},
    {TOKEN_TYPE, "type"},
    {TOKEN_MATRIX, "matrix"},
    {TOKEN_MAP, "map"},
    {TOKEN_WEAK, "weak"},
    {TOKEN_INT8, "int8"},
    {TOKEN_INT16, "int16"},
    {TOKEN_INT32, "int32"},
    {TOKEN_INT64, "int64"},
    {TOKEN_UINT8, "uint8"},
    {TOKEN_UINT16, "uint16"},
    {TOKEN_UINT32, "uint32"},
    {TOKEN_UINT64, "uint64"},
    {TOKEN_FLOAT32, "float32"},
    {TOKEN_FLOAT64, "float64"},
    {TOKEN_COMPLEX64, "complex64"},
    {TOKEN_COMPLEX128,"complex128"},
    {TOKEN_STRING, "string"},
    {TOKEN_RUNE, "rune"},
    {TOKEN_BOOL, "bool"},
    {TOKEN_SIZE_T, "size_t"},
    {TOKEN_ERRORCODE, "errcode"},
    
    {TOKEN_FUNC, "func"},
    {TOKEN_PURE, "pure"},
    {TOKEN_IN, "in"},
    {TOKEN_OUT, "out"},
    {TOKEN_IO, "io" },
    {TOKEN_ETC, "..."},
    {TOKEN_IF, "if"},
    {TOKEN_ELSE, "else"},
    {TOKEN_WHILE, "while"},
    {TOKEN_FOR, "for"},
    {TOKEN_RETURN, "return"},
    {TOKEN_BREAK, "break"},
    {TOKEN_CONTINUE, "continue"},
    {TOKEN_ERROR, "error"},
    {TOKEN_CLEANUP, "cleanup"},
    {TOKEN_RUN, "run"},
    {TOKEN_BIND, "bind"},
    
    {TOKEN_SIZEOF, "sizeof"},
    {TOKEN_DIMOF, "dimof"},
    {TOKEN_XOR, "xor"},
    {TOKEN_AS, "as"},
    {TOKEN_TYPESWITCH, "typeswitch"},
    {TOKEN_CASE, "case"},
    {TOKEN_DEFAULT, "default"},
    
    {TOKEN_PUBLIC, "public"},
    {TOKEN_PRIVATE, "private"},
    {TOKEN_ENUM, "enum"},
    {TOKEN_STRUCT, "struct"},
    {TOKEN_CLASS, "class"},
    {TOKEN_FINALIZE, "finalize"},
    {TOKEN_INTERFACE, "interface"},
    {TOKEN_STATIC, "static"},
    {TOKEN_IMPLEMENTS, "implements"},
    {TOKEN_BYPASS, "bypass"},
    {TOKEN_TEMPLATE, "template"},
    {TOKEN_ARGUMENT, "argument"},
    {TOKEN_VOLATILE, "volatile"},
    {TOKEN_THROW, "throw"},
    {TOKEN_TRY, "try"},
    {TOKEN_CATCH, "catch"},
    {TOKEN_STEP, "step"},
    
    {TOKEN_ROUND_OPEN, "("},
    {TOKEN_ROUND_CLOSE, ")"},
    {TOKEN_SQUARE_OPEN, "["},
    {TOKEN_SQUARE_CLOSE, "]"},
    {TOKEN_CURLY_OPEN, "{"},
    {TOKEN_CURLY_CLOSE, "}"},
    {TOKEN_ANGLE_OPEN_LT, "<"},
    {TOKEN_ANGLE_CLOSE_GT, ">"},
    
    {TOKEN_COMMA, ","},
    {TOKEN_ASSIGN, "="},
    {TOKEN_INC, "++"},
    {TOKEN_DEC, "--"},
    {TOKEN_DOT, "."},
    {TOKEN_PLUS, "+"},
    {TOKEN_MINUS, "-"},
    {TOKEN_MPY, "*"},
    {TOKEN_DIVIDE, "/"},
    {TOKEN_POWER, "^"},
    {TOKEN_MOD, "%"},
    {TOKEN_SHR, ">>"},
    {TOKEN_SHL, "<<"},
    {TOKEN_NOT, "~"},
    {TOKEN_AND, "&"},
    {TOKEN_OR, "|"},
    //{TOKEN_GT, ">"},  // conflicts with brakets
    {TOKEN_GTE, ">="},
    //{TOKEN_LT, "<"},
    {TOKEN_LTE, "<="},
    {TOKEN_DIFFERENT, "!="},
    {TOKEN_EQUAL, "=="},
    {TOKEN_LOGICAL_NOT, "!"},
    {TOKEN_LOGICAL_AND, "&&"},
    {TOKEN_LOGICAL_OR, "||"},
    {TOKEN_COLON, ":"},
    {TOKEN_SEMICOLON, ";"},

    {TOKEN_UPD_PLUS, "+=" },
    {TOKEN_UPD_MINUS, "-=" },
    {TOKEN_UPD_MPY, "*=" },
    {TOKEN_UPD_DIVIDE, "/=" },
    {TOKEN_UPD_POWER, "^=" },
    {TOKEN_UPD_MOD, "%=" },
    {TOKEN_UPD_SHR, ">>=" },
    {TOKEN_UPD_SHL, "<<=" },
    {TOKEN_UPD_AND, "&=" },
    {TOKEN_UPD_OR, "|=" },
    {TOKEN_UPD_LOGICAL_AND, "&&=" },
    {TOKEN_UPD_LOGICAL_OR, "||=" },
};

Lexer::Lexer()
{
    int         ii, ash;
    int         num_tokens = sizeof(keywords) / sizeof(keywords[0]);
    TokenDesc   *td;

    m_fd = NULL;

    for (ii = 0; ii < TOKENS_COUNT; ++ii) {
        ash_next_item[ii] = -1;
        token_to_string[ii] = "";
    }
    for (ii = 0; ii < TABLE_SIZE; ++ii) {
        ash_table[ii] = -1;
    }

    for (ii = 0; ii < num_tokens; ++ii) {

        // token to string
        td = &keywords[ii];
        token_to_string[td->token] = td->token_string;

        // string to ash
        ash = ComputeAsh(td->token_string);

        // ash to token
        if (ash_table[ash] != -1) {
            ash_next_item[td->token] = ash_table[ash];
        } 
        ash_table[ash] = td->token;
    }

    // just to know
    /*
    int max_len, len, jj

    max_len = 1;
    for (ii = 0; ii < TOKENS_COUNT; ++ii) {
        len = 1;
        jj = ash_next_item[ii];
        while (jj != -1) {
            jj = ash_next_item[jj];
            ++len;
        }
        if (len > max_len) {
            max_len = len;
        }
    }
    ++max_len;  // place a breakpoint here
    */
}

Lexer::~Lexer()
{
    if (m_fd != NULL) {
        fclose(m_fd);
    }
}

int Lexer::ComputeAsh(const char *symbol)
{
    int ash = 0;
    int shift = 0;
    int tmp;

    while (*symbol) {

        // actually a roll left
        tmp = (*symbol) << shift;
        ash ^= tmp & ASH_MASK;
        ash ^= tmp >> ASH_BITS;

        ++symbol;
        shift += 7;
        if (shift > ASH_BITS) shift -= ASH_BITS;
    }
    return(ash & ASH_MASK);
}

// init
int Lexer::OpenFile(const char *filename)
{
    m_fd = fopen(filename, "rb");
    if (m_fd == NULL) return(FAIL);
    m_status = LS_REGULAR;
    m_curline = 0;          // as it reads the first line, advances to 1 (usually lines are numbered from 1)
    m_curcol = 0;
    return(0);
}

void Lexer::CloseFile(void)
{
    fclose(m_fd);
    m_fd = NULL;
}

// all this decoding fuss to return a correct column position.
int Lexer::GetNewLine(void)
{
    int ch;

    // skip empty lines
    m_curcol = 0;
    m_line_buffer.clear();
    while (m_line_buffer.size() == 0) { 

        // skip empty lines
        m_tmp_line_buffer = "";
        while (m_tmp_line_buffer.length() == 0) {

            // collect a line
            while (true) {
                ch = getc(m_fd);

                // detect eof
                if (ch == EOF) {
                    if (m_tmp_line_buffer.length() == 0) {
                        return(EOF);
                    }
                    break;
                }

                // detect eol
                if (ch == '\r') {
                    ch = getc(m_fd);
                    if (ch != '\n') {
                        ungetc(ch, m_fd);
                    }
                    break;
                } else if (ch == '\n') {
                    break;
                }

                // expand tabs
                if (ch == '\t') {
                    m_tmp_line_buffer += "    ";
                } else {
                    m_tmp_line_buffer += ch;
                }
            }
            ++m_curline;
        }

        m_tmp_line_buffer.utf8_decode(&m_line_buffer);

        // pop the terminator
        m_line_buffer.pop_back();
    }
    return(0);
}

Token Lexer::Advance(void)
{
    int32_t ch, len;

    if (m_status == LS_EOF) return(m_curr_token);
    m_curr_token = TOKENS_COUNT;
    while (m_curr_token == TOKENS_COUNT) {    // i.e. while not found/assigned
        if (m_curcol >= (int)m_line_buffer.size()) {
            if (GetNewLine() == EOF) {
                m_curr_token = TOKEN_EOF;
                m_status = LS_EOF;
                return(m_curr_token);
            }
        }

        // optimism ! if we don't find a token gets overwritten on next iteration
        m_curr_token_row = m_curline;
        m_curr_token_col = m_curcol;
        m_curr_token_string = "";

        ch = m_line_buffer[m_curcol++];
        if (ch == '\'') {
            m_curr_token = TOKEN_LITERAL_UINT;
            ReadCharacterLiteral();
        } else if (ch == '\"') {
            m_curr_token = TOKEN_LITERAL_STRING;
            ReadStringLiteral();
        } else if (ch >= '0' && ch <= '9') {
            --m_curcol;
            ReadNumberLiteral();
        } else if (isalpha(ch) || ch == '_') {
            --m_curcol;
            ReadName();
        } else if (ispunct(ch)) {
            --m_curcol;
            ReadSymbol();
        } else if (!iswspace(ch)) {
            Error(UNEXPECTED_CHAR, m_curcol-1);
        }
    }
    if (m_curr_token != TOKEN_COMMENT) {
        m_curr_token_verbatim = "";
        len = m_curcol - m_curr_token_col;
        if (len > 0) {
            m_curr_token_verbatim.utf8_encode(&m_line_buffer[m_curr_token_col], len);
        }
    }
    return(m_curr_token);
}

void Lexer::ClearError(void)
{
    while (m_curcol < (int)m_line_buffer.size() && !iswspace(m_line_buffer[m_curcol])) {
        ++m_curcol;
    }
}

void Lexer::ReadCharacterLiteral(void)
{
    int32_t ch;

    if (ResidualCharacters() < 1) Error(LE_TRUNCATED_CONSTANT, m_curcol-1);
    ch = m_line_buffer[m_curcol++];
    if (ch == '\\') {
        ch = ReadEscapeSequence();
    }
    m_curr_token_string = "";
    m_curr_token_string.utf8_encode(&ch, 1);
    if (m_line_buffer.size() == m_curcol) Error(LE_TRUNCATED_CONSTANT, m_curcol - 1);
    if (m_line_buffer[m_curcol] != '\'') Error(LE_APEX_EXPECTED, m_curcol);
    m_curcol++;
}

void Lexer::ReadStringLiteral(void)
{
    int32_t ch;
    m_curr_token_string = "";
    while (true) {
        if (m_line_buffer.size() == m_curcol) Error(LE_TRUNCATED_STRING, m_curcol - 1);
        ch = m_line_buffer[m_curcol++];
        if (ch == '\"') {
            return;
        } else if (ch == '\\') {
            ch = ReadEscapeSequence();
        }
        m_curr_token_string.utf8_encode(&ch, 1);
    }
}

int32_t Lexer::ReadEscapeSequence(void)
{
    int ch;

    if (m_line_buffer.size() == m_curcol) Error(LE_WRONG_ESCAPE_SEQUENCE, m_curcol-1);
    ch = m_line_buffer[m_curcol++];
    switch (ch) {
    case '\'':
    case '\"':
    case '\\':
        return(ch);
    case '?':
        return('?');
    case 'a':
        return('\a');
    case 'b':
        return('\b');
    case 'f':
        return('\f');
    case 'n':
        return('\n');
    case 'r':
        return('\r');
    case 't':
        return('\t');
    case 'v':
        return('\v');
    case 'x':
    case 'u':
        if (m_line_buffer.size() - m_curcol < 1)  Error(LE_WRONG_ESCAPE_SEQUENCE, m_curcol - 1);
        ch = HexToChar(&m_line_buffer[m_curcol], MIN(6, ResidualCharacters()));
        return(ch);
    default:
        break;
    }
    --m_curcol; 
    Error(LE_WRONG_ESCAPE_SEQUENCE, m_curcol);
    return(0);  // unreachable
}

int32_t Lexer::HexToChar(int32_t *cps, int maxlength)
{
    int digit, value;
    int retval = 0;
    
    for (digit = 0; digit < maxlength; ++digit) {
        value = cps[digit];
        if (value >= '0' && value <= '9') {
            retval = (retval << 4) + value - '0';
        } else if (value >= 'a' && value <= 'f') {
            retval = (retval << 4) + value + (10 - 'a');
        } else if(value >= 'A' && value <= 'F') {
            retval = (retval << 4) + value + (10 - 'A');
        } else if (digit == 0) {
            Error(LE_WRONG_ESCAPE_SEQUENCE, m_curcol);
        } else {
            m_curcol += digit;
            return(retval);
        }
    }
    return(retval);
}

void Lexer::ReadNumberLiteral(void)
{
    int32_t ch;

    if (ResidualCharacters() >= 2 && m_line_buffer[m_curcol] == '0' && m_line_buffer[m_curcol + 1] == 'x') {
        m_curcol += 2;
        m_uint_real = ReadHexLiteral();
        if (ResidualCharacters() > 0) {
            ch = m_line_buffer[m_curcol];
            if (ch == 'i' || ch == 'I') {
                ++m_curcol;
                m_curr_token = TOKEN_LITERAL_IMG;
            }
        }
        if (ResidualCharacters() > 0) {
            ch = m_line_buffer[m_curcol];
            if (!ispunct(ch) && !isspace(ch)) {
                Error(LE_NUM_TERMINATION, m_curcol);
            }
        }
    } else {
        ReadDecimalLiteral();
    }
}

enum NumberSMState {NSM_INTEGER, NSM_FRACT0, NSM_FRACT, NSM_EXP_SIGN, NSM_EXP0, NSM_EXP, NSM_TERM};

void Lexer::ReadDecimalLiteral(void)
{
    NumberSMState   state = NSM_INTEGER;
    int             dst_index = 0;
    int             decimal_point = 0;
    int             exponent = 0;
    int             exponent_digits = 0;
    bool            imaginary = false;
    bool            negative_exponent = false;
    bool            done = false;
    int32_t         ch;
    int             scan;
    char            buffer[100];

    while (m_curcol < (int)m_line_buffer.size() && !done) {
        if (dst_index == sizeof(buffer)) {
            Error(LE_NUM_TOO_MANY_DIGITS, m_curcol);
        }
        ch = m_line_buffer[m_curcol++];
        switch (state) {
        case NSM_INTEGER:
            if (ch >= '0' && ch <= '9') {
                buffer[dst_index++] = ch;
            } else if (ch == '.') {
                decimal_point = dst_index;  // digits before decimal point
                state = NSM_FRACT0;
            } else if (ch == 'e' || ch == 'E') {
                state = NSM_EXP_SIGN;
            } else if (ch == 'i' || ch == 'I') {
                imaginary = true;
                state = NSM_TERM;
            } else {
                --m_curcol;
                state = NSM_TERM;
            }
            break;
        case NSM_FRACT0:                // stil waiting for the first digit
            if (ch >= '0' && ch <= '9') {
                buffer[dst_index++] = ch;
                state = NSM_FRACT;
            } else {
                --m_curcol;
                Error(LE_EXPECTED_A_DIGIT, m_curcol);
            }
            break;
        case NSM_FRACT:
            if (ch >= '0' && ch <= '9') {
                buffer[dst_index++] = ch;
            } else if (ch == 'e' || ch == 'E') {
                state = NSM_EXP_SIGN;
            } else if (ch == 'i' || ch == 'I') {
                imaginary = true;
                state = NSM_TERM;
            } else {
                --m_curcol;
                state = NSM_TERM;
            }
            break;
        case NSM_EXP_SIGN:
            if (ch >= '0' && ch <= '9') {
                exponent = ch - '0';
                ++exponent_digits;
                state = NSM_EXP;
            } else if (ch == '+') {
                state = NSM_EXP0;
            } else if (ch == '-') {
                negative_exponent = true;
                state = NSM_EXP0;
            } else {
                --m_curcol;
                Error(LE_EXPECTED_A_DIGIT, m_curcol);
            }
            break;
        case NSM_EXP0:                // stil waiting for the first digit
            if (ch >= '0' && ch <= '9') {
                exponent = ch - '0';
                ++exponent_digits;
                state = NSM_EXP;
            } else {
                --m_curcol;
                Error(LE_EXPECTED_A_DIGIT, m_curcol);
            }
            break;
        case NSM_EXP:
            if (ch >= '0' && ch <= '9') {
                exponent = exponent * 10 + ch - '0';
                ++exponent_digits;
                if (exponent > 500) {
                    Error(LS_CONST_VALUE_TOO_BIG, (m_curcol + m_curr_token_col) >> 1);
                }
            } else if (ch == 'i' || ch == 'I') {
                imaginary = true;
                state = NSM_TERM;
            } else {
                --m_curcol;
                state = NSM_TERM;
            }
            break;
        case NSM_TERM:
            --m_curcol;
            if (ch != '.' && (isspace(ch) || ispunct(ch))) {
                done = true;
            } else {
                Error(LE_NUM_TERMINATION, m_curcol);
            }
            break;
        }
    }

    if (state == NSM_FRACT0 || state == NSM_EXP_SIGN || state == NSM_EXP0) {
        Error(LE_TRUNCATED_CONSTANT, m_curcol - 1);
    }

    // no decimal point is the same as decimal point after all the digits.
    if (decimal_point == 0) decimal_point = dst_index;

    // ignore the leading zeros
    for (scan = 0; scan < dst_index - 1 && buffer[scan] == '0'; ++scan);
    if (scan != 0) {
        for (int ii = scan; ii < dst_index; ++ii) buffer[ii - scan] = buffer[ii];
        decimal_point -= scan;
        dst_index -= scan;
    }

    // classify and check the range
    buffer[dst_index] = 0;
    if (decimal_point == dst_index && exponent_digits == 0 && !imaginary && strcmp(buffer, "18446744073709551616") < 0) {
        m_curr_token = TOKEN_LITERAL_UINT;
        return;
    }

    // normalize fraction and check magnitude
    if (negative_exponent) exponent = -exponent;
    exponent += decimal_point - 1;  // leave just one digit before the point
    if (exponent > 308 || exponent == 308 && strcmp(buffer, "17976931348623158") > 0) {
        Error(LS_CONST_VALUE_TOO_BIG, (m_curcol + m_curr_token_col) >> 1);
    }
    
    if (imaginary) {
        m_curr_token = TOKEN_LITERAL_IMG;
    } else {
        m_curr_token = TOKEN_LITERAL_FLOAT;
    }
}

uint64_t Lexer::ReadHexLiteral(void)
{
    int         digit, value;
    uint64_t    retval = 0;

    for (digit = 0; digit < 17 && m_curcol < (int)m_line_buffer.size(); ++digit) {
        value = m_line_buffer[m_curcol++];
        if (value >= '0' && value <= '9') {
            retval = (retval << 4) + value - '0';
        } else if (value >= 'a' && value <= 'f') {
            retval = (retval << 4) + value + (10 - 'a');
        } else if (value >= 'A' && value <= 'F') {
            retval = (retval << 4) + value + (10 - 'A');
        } else {
            --m_curcol; // not part of the number
            if (digit == 0) {
                Error(LE_HEX_CONST_ERROR, m_curcol);
            }
            break;
        }
    }
    if (digit == 17) {
        Error(LS_CONST_VALUE_TOO_BIG, m_curcol - 8);
    }
    m_curr_token = TOKEN_LITERAL_UINT;
    return(retval);
}

void Lexer::ReadName(void)
{
    int     numchars = ResidualCharacters();
    int     digit, ash;
    int32_t ch;

    m_curr_token_string = "";
    m_curr_token_string += m_line_buffer[m_curcol++];
    for (digit = 1; digit < numchars; ++digit) {
        ch = m_line_buffer[m_curcol];
        if (isalpha(ch) || isdigit(ch) || ch == '_') {
            m_curr_token_string += ch;
            ++m_curcol;
        } else {
            break;
        }
    }
    if (m_curr_token_string[0] != '_') {
        ash = ComputeAsh(m_curr_token_string.c_str());
        m_curr_token = AshLookUp(ash, m_curr_token_string.c_str());
    } else {
        m_curr_token = TOKEN_NAME;
    }
}

void Lexer::ReadSymbol(void)
{
    int         numchars = ResidualCharacters();
    int         digit, tmp, shift, ash;
    bool        found = false;
    int32_t     ch;
    Token       token;

    m_curr_token_string = "";
    shift = ash = 0;
    for (digit = 0; digit < numchars; ++digit) {
        ch = m_line_buffer[m_curcol++];
        m_curr_token_string += ch;

        if (ch > 128) break;
        if (isspace(ch)) {
            --m_curcol;     // else ErrorClear consumes also the next token
            break;
        }

        // actually a roll left
        tmp = ch << shift;
        ash ^= tmp & ASH_MASK;
        ash ^= tmp >> ASH_BITS;

        shift += 7;
        if (shift > ASH_BITS) shift -= ASH_BITS;

        token = AshLookUp(ash, m_curr_token_string.c_str());
        if (token == TOKEN_NAME) { 

            // if there was a match and now there is no match, discard the last char and exit.
            if (found) {            
                m_curr_token_string.erase(m_curr_token_string.size() - 1);
                --m_curcol;
                break;
            }
        } else {

            // there is a match. Let's go on anyway to see if this is only a part of the whole.  
            m_curr_token = token;
            found = true;
        }
    }
    if (!found) {
        Error(LS_ILLEGAL_NAME, m_curcol - 1);
    }
    if (m_curr_token == TOKEN_COMMENT) {
        ReadComment();
    } else if (m_curr_token == TOKEN_INLINE_COMMENT) {
        m_curr_token_string.utf8_encode(&m_line_buffer[m_curcol], ResidualCharacters());
        m_curcol = m_line_buffer.size();
    }
}

void Lexer::ReadComment(void)
{
    int     depth = 1;
    int     status = 0;
    int32_t ch;

    m_curr_token_verbatim = "\\*";
    while (depth > 0) {
        if (m_curcol == m_line_buffer.size()) {
            if (GetNewLine() == EOF) {
                Error(UNEXPECTED_EOF, m_line_buffer.size() - 1);
                m_status = LS_EOF;
                return;
            }
            status = 0;
            m_curr_token_verbatim += "\r\n";
        }
        ch = m_line_buffer[m_curcol++];
        m_curr_token_string.utf8_encode(&ch, 1);
        m_curr_token_verbatim.utf8_encode(&ch, 1);
        switch (status) {
        case '/':
            if (ch == '*') {
                ++depth;
                status = 0;
            } else if (ch != status) {
                status = 0;
            }
            break;
        case '*':
            if (ch == '/') {
                --depth;
                status = 0;
            } else if (ch != status) {
                status = 0;
            }
            break;
        default:
            if (ch == '/' || ch == '*') {
                status = ch;
            }
        }
    }
}

Token Lexer::AshLookUp(int ash, const char *name)
{
    int token = ash_table[ash];
    while (token >= 0) {
        if (strcmp(token_to_string[token], name) == 0) return((Token)token);
        token = ash_next_item[token];
    }
    return(TOKEN_NAME);
}

static const char *error_desc[] = {
    "truncated numeric literal",
    "unknown/wrong escape sequence",
    "expecting \"\'\"",
    "truncated string",
    "expecting an hexadecimal digit",
    "literal value too big to fit in its type",
    "illegal name: only alpha ,digits and \'_\' are allowed.",
    "unexpected char",
    "unexpected end of file",
    "numeric literals must be terminated by blank or punctuation (except \'.\')",
    "too many digits in number",
    "expected a digit"
};

void Lexer::Error(LexerError error, int column)
{
    throw(ParsingException(error, m_curline, MAX(column, 0) + 1, error_desc[error]));
}

} // namespace