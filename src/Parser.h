#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast_nodes.h"

namespace StayNames {

class Parser {
    Lexer *m_lexer;
    Token m_token;

    Token   Advance(void);
    void    Error(const char *message);

    // these functions gets called with the first token of the parsed term already in m_token,
    // all of them return with the first not-parsed token in m_token
    // if they are called, the term they parse is present, either they throw an error or are succesfull.
    // NOTE: the functions must check m_token unless they are absolutely sure it has been checked by the caller.
    // es: ParseVar() gets called if the keyword 'var' is in m_token and a var declaration is present for sure.
    void                    ParseDependency(AstFile *file);
    void                    ParseDeclaration(AstFile *file);
    IAstNode                *ParseVar(void);
    IAstNode                *ParseTypeSpecification(void);
    FuncDeclaration         *ParseFunctionDeclaration(void);
    void                    ParseFullName(string *part1, string *part2);           // may be qualified
    AstArrayOrMatrixType    *ParseIndices(bool ismatrix);
    AstIniter               *ParseIniter(void);
    AstFuncType             *ParseFunctionType(void);
    void                    ParseArgsDef(AstFuncType *desc);
    AstArgumentDecl         *ParseSingleArgDef(void);
    AstBlock                *ParseBlock(void);
    IAstNode                *ParseLeftTermStatement(void);
    IAstNode                *ParseExpression(void);
    IAstNode                *ParseLeftTerm(const char *errmess = NULL);
    void ParseIndices(void);
    void ParseArguments(void);
public:
    Parser();
    ~Parser();

    void Init(Lexer *lexer);
    AstFile *ParseAll(void);
};

}

#endif
