#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <string.h>
#include "string"
#include "vector.h"

namespace SingNames {

enum Token {
    TOKEN_COMMENT,
    TOKEN_INLINE_COMMENT,
    TOKEN_NAME,
    TOKEN_LITERAL_FLOAT,
    TOKEN_LITERAL_UINT,
    TOKEN_LITERAL_IMG,
    TOKEN_LITERAL_STRING,
    TOKEN_EOF,
    TOKEN_EMPTY_LINES,

    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_VOID,

    TOKEN_MUT,
    TOKEN_REQUIRES,
    TOKEN_NAMESPACE,
    TOKEN_VAR,
    TOKEN_CONST,
    TOKEN_TYPE,
    TOKEN_MAP,
    TOKEN_WEAK,
    TOKEN_INT8,
    TOKEN_INT16,
    TOKEN_INT32,
    TOKEN_INT64,
    TOKEN_UINT8,
    TOKEN_UINT16,
    TOKEN_UINT32,
    TOKEN_UINT64,
    TOKEN_FLOAT32,
    TOKEN_FLOAT64,
    TOKEN_COMPLEX64,
    TOKEN_COMPLEX128,
    TOKEN_LET,
    TOKEN_STRING,
    TOKEN_BOOL,

    TOKEN_FUNC,
    TOKEN_PURE,
    TOKEN_IN,
    TOKEN_OUT,
    TOKEN_IO,
    TOKEN_DOUBLEDOT,    // triky - used to avoid giving up the TOKEN_ETC parsing
    TOKEN_ETC,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,

    TOKEN_SIZEOF,
    TOKEN_XOR,
    TOKEN_CASE,
    TOKEN_TYPESWITCH,
    TOKEN_SWITCH,
    TOKEN_DEFAULT,

    TOKEN_PUBLIC,
    TOKEN_PRIVATE,
    TOKEN_ENUM,
    TOKEN_CLASS,
    TOKEN_THIS,
    TOKEN_INTERFACE,
    TOKEN_BY,
    TOKEN_STEP,
    TOKEN_MIN,
    TOKEN_MAX,
    TOKEN_SWAP,
    
    TOKEN_ROUND_OPEN,
    TOKEN_ROUND_CLOSE,
    TOKEN_SQUARE_OPEN,
    TOKEN_SQUARE_CLOSE,
    TOKEN_CURLY_OPEN,
    TOKEN_CURLY_CLOSE,
    TOKEN_ANGLE_OPEN_LT,
    TOKEN_ANGLE_CLOSE_GT,

    TOKEN_COMMA,
    TOKEN_ASSIGN,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_DOT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MPY,
    TOKEN_DIVIDE,
    TOKEN_POWER,
    TOKEN_MOD,
    TOKEN_SHR,
    TOKEN_SHL,
    TOKEN_NOT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_GTE,
    TOKEN_LTE,
    TOKEN_DIFFERENT,
    TOKEN_EQUAL,
    TOKEN_LOGICAL_NOT,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_COLON,
    TOKEN_SEMICOLON,

    TOKEN_UPD_PLUS,
    TOKEN_UPD_MINUS,
    TOKEN_UPD_MPY,
    TOKEN_UPD_DIVIDE,
    TOKEN_UPD_XOR,
    TOKEN_UPD_MOD,
    TOKEN_UPD_SHR,
    TOKEN_UPD_SHL,
    TOKEN_UPD_AND,
    TOKEN_UPD_OR,
    TOKEN_OUT_OPT,
    TOKEN_DEF,
    TOKEN_UNDERSCORE,
    TOKEN_TRY,

    TOKEN_UNUSED,

    TOKENS_COUNT
};

enum LexerStatus {LS_REGULAR, LS_COMMENT, LS_EOF};

enum LexerError {LE_TRUNCATED_CONSTANT, LE_WRONG_ESCAPE_SEQUENCE, LE_APEX_EXPECTED, LE_TRUNCATED_STRING,
                 LE_HEX_CONST_ERROR, LS_CONST_VALUE_TOO_BIG, LS_ILLEGAL_NAME, UNEXPECTED_CHAR, UNEXPECTED_EOF,
                 LE_NUM_TERMINATION, LE_NUM_TOO_MANY_DIGITS, LE_EXPECTED_A_DIGIT, LE_DOUBLE_UNDERSCORE, 
                 LE_UNDERSCORE_UNALLOWED, LE_RESERVED_SYMBOL};

class Lexer {
    static const int ASH_BITS = 11;
    static const int TABLE_SIZE = (1 << ASH_BITS);
    static const int ASH_MASK = (TABLE_SIZE - 1);

    // ash table of interpunctuation and ketworks
    static int             ash_table[];
    static int             ash_next_item[];
    static const char      *token_to_string[];
    static bool            ash_table_inited;

    char  *source_;
    const char  *scan_;

    // info on available token
    Token           m_curr_token;
    string          m_curr_token_string;
    int             m_curr_token_row;
    int             m_curr_token_col;
    int             m_curr_token_last_row;
    int             m_curr_token_last_col;
    string          m_curr_token_verbatim;
    uint64_t        m_uint_real;
    double          m_float_real, m_float_img;

    string          m_tmp_line_buffer;
    vector<int32_t> m_line_buffer;
    int             m_curcol;       // 0 based (index into m_line_buffer)
    int             m_curline;      // 1 based

    LexerStatus     m_status;
    int             m_tokens_on_line;

    // last error
    LexerError      m_error;
    int             m_error_row;
    int             m_error_column;

    int         ComputeAsh(const char *symbol);
    int         GetNewLine(void);
    bool        IsEmptyLine(vector<int32_t> *line);
    bool        ReadStringLiteral(void);
    bool        ReadEscapeSequence(int32_t *value);
    bool        HexToChar(int32_t *retv, int length);
    bool        ReadNumberLiteral(void);
    bool        ReadDecimalLiteral(void);
    bool        ReadHexLiteral(uint64_t *retv);
    bool        ReadSymbol(void);
    bool        ReadName(void);
    Token       AshLookUp(int ash, const char *name);
    bool        ReadComment(void);
    void        Error(LexerError error, int column);
    inline int  ResidualCharacters(void) { return(m_line_buffer.size() - m_curcol); }
    inline int  CompareIntRep(const char *a, const char *b) {
        int diff = strlen(a) - strlen(b);
        if (diff < 0) return(-1);
        if (diff > 0) return(1);
        return(strcmp(a, b));
    }
public:
    Lexer();
    ~Lexer();

    // init
    int OpenFile(const char *filename);
    void Init(const char *source);

    // examining the next to come element
    Token CurrToken(void) { return(m_curr_token); }
    const char *CurrTokenString(void) { return(m_curr_token_string.c_str()); }  // not available for numeric literals
    int  CurrTokenLine(void) { return(m_curr_token_row); }
    int  CurrTokenColumn(void) { return(m_curr_token_col); }
    int  CurrTokenLastLine(void) { return(m_curr_token_last_row); }
    int  CurrTokenLastColumn(void) { return(m_curr_token_last_col); }
    const char *CurrTokenVerbatim(void) { return(m_curr_token_verbatim.c_str()); }
    int TokensOnThisLine(void) { return(m_tokens_on_line); }

    // step forward and return the current
    bool Advance(Token *token);

    // the power operator '**' is context-sensitive. Only in some places the parser asks
    // if a TOKEN_MPY can be converted to TOKEN_POWER based on the fact it is followed by another '*' without
    // intermixed spaces.
    void ConvertToPower(Token *token);

    void GetError(string *error_str, int32_t *row, int32_t *col);
    void ClearError(void);

    // utils
    static const char *GetTokenString(Token token);
    int GetBinopPriority(Token token);

    static const int max_priority = 5;
};

}

#endif