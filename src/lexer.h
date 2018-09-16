#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "string"
#include "vector.h"

namespace StayNames {

enum Token {
    TOKEN_COMMENT,
    TOKEN_INLINE_COMMENT,
    TOKEN_NAME,
    TOKEN_LITERAL_FLOAT,
    TOKEN_LITERAL_UINT,
    TOKEN_LITERAL_IMG,
    TOKEN_LITERAL_STRING,
    TOKEN_EOF,

    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_VOID,

    TOKEN_PACKAGE,
    TOKEN_REQUIRES,
    TOKEN_VAR,
    TOKEN_CONST,
    TOKEN_TYPE,
    TOKEN_MATRIX,
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
    TOKEN_STRING,
    TOKEN_RUNE,
    TOKEN_BOOL,
    TOKEN_SIZE_T,
    TOKEN_ERRORCODE,

    TOKEN_FUNC,
    TOKEN_PURE,
    TOKEN_IN,
    TOKEN_OUT,
    TOKEN_IO,
    TOKEN_ETC,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_ERROR,
    TOKEN_CLEANUP,
    TOKEN_RUN,
    TOKEN_BIND,

    TOKEN_SIZEOF,
    TOKEN_DIMOF,
    TOKEN_XOR,
    TOKEN_AS,
    TOKEN_TYPESWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,

    TOKEN_PUBLIC,
    TOKEN_PRIVATE,
    TOKEN_ENUM,
    TOKEN_STRUCT,
    TOKEN_CLASS,
    TOKEN_FINALIZE,
    TOKEN_INTERFACE,
    TOKEN_STATIC,
    TOKEN_IMPLEMENTS,
    TOKEN_BYPASS,
    TOKEN_TEMPLATE,
    TOKEN_ARGUMENT,
    TOKEN_VOLATILE,
    TOKEN_THROW,
    TOKEN_TRY,
    TOKEN_CATCH,

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
    //TOKEN_GT,
    TOKEN_GTE,
    //TOKEN_LT,
    TOKEN_LTE,
    TOKEN_DIFFERENT,
    TOKEN_EQUAL,
    TOKEN_LOGICAL_NOT,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_COLON,

    TOKEN_UPD_PLUS,
    TOKEN_UPD_MINUS,
    TOKEN_UPD_MPY,
    TOKEN_UPD_DIVIDE,
    TOKEN_UPD_POWER,
    TOKEN_UPD_MOD,
    TOKEN_UPD_SHR,
    TOKEN_UPD_SHL,
    TOKEN_UPD_AND,
    TOKEN_UPD_OR,
    TOKEN_UPD_LOGICAL_AND,
    TOKEN_UPD_LOGICAL_OR,

    TOKENS_COUNT
};

enum LexerStatus {LS_REGULAR, LS_COMMENT, LS_EOF};

enum LexerError {LE_TRUNCATED_CONSTANT, LE_WRONG_ESCAPE_SEQUENCE, LE_APEX_EXPECTED, LE_TRUNCATED_STRING,
                 LE_HEX_CONST_ERROR, LS_CONST_VALUE_TOO_BIG, LS_ILLEGAL_NAME, UNEXPECTED_CHAR, UNEXPECTED_EOF,
                 LE_NUM_TERMINATION, LE_NUM_TOO_MANY_DIGITS, LE_EXPECTED_A_DIGIT};

class Lexer {
    static const int ASH_BITS = 10;
    static const int TABLE_SIZE = (1 << ASH_BITS);
    static const int ASH_MASK = (TABLE_SIZE - 1);

    FILE            *m_fd;
    int             ash_table[TABLE_SIZE];
    int             ash_next_item[TOKENS_COUNT];
    const char      *token_to_string[TOKENS_COUNT];

    // info on available token
    Token           m_curr_token;
    string          m_curr_token_string;
    int             m_curr_token_row;
    int             m_curr_token_col;
    string          m_curr_token_verbatim;
    uint64_t        m_uint_real;
    double          m_float_real, m_float_img;

    string          m_tmp_line_buffer;
    vector<int32_t> m_line_buffer;
    int             m_curcol;       // 0 based (index into m_line_buffer)
    int             m_curline;      // 1 based

    LexerStatus     m_status;

    int         ComputeAsh(const char *symbol);
    int         GetNewLine(void);
    void        ReadCharacterLiteral(void);
    void        ReadStringLiteral(void);
    int32_t     ReadEscapeSequence(void);
    int32_t     HexToChar(int32_t *cps, int maxlength);
    void        ReadNumberLiteral(void);
    void        ReadDecimalLiteral(void);
    uint64_t    ReadHexLiteral(void);
    void        ReadSymbol(void);
    void        ReadName(void);
    Token       AshLookUp(int ash, const char *name);
    void        ReadComment(void);
    void        Error(LexerError error, int column);
    inline int  ResidualCharacters(void) { return(m_line_buffer.size() - m_curcol); }
public:
    Lexer();
    ~Lexer();

    // init
    int OpenFile(const char *filename);
    void CloseFile(void);

    // examining the next to come element
    Token CurrToken(void) { return(m_curr_token); }
    const char *CurrTokenString(void) { return(m_curr_token_string.c_str()); }  // not available for numeric literals
    int  CurrTokenLine(void) { return(m_curr_token_row); }
    int  CurrTokenColumn(void) { return(m_curr_token_col); }
    const char *CurrTokenVerbatim(void) { return(m_curr_token_verbatim.c_str()); }

    // step forward and return the current
    Token Advance(void);

    void ClearError(void);
};

}

#endif