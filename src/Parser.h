#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast_nodes.h"
#include "NamesList.h"

namespace SingNames {

class Parser {
    Lexer *m_lexer;
    Token m_token;

    bool        on_error_;      // needs to recover skipping some stuff
    bool        has_errors_;    // found in previous blocks/declarations.
    NamesList   errors_;

    void    CheckSemicolon(void);
    Token   Advance(void);
    void    Error(const char *message);                         // throws and need to recover
    void    SetError(const char *message, int row, int column); // just adds to the errors list
    bool    SkipToNextStatement(void);                          // returns false if exits from the function/block
    void    SkipToNextDeclaration(void);
    bool    OnDeclarationToken(void);
    bool    OutOfFunctionToken(void);
    Token   SkipToken(void);

    // these functions gets called with the first token of the parsed term already in m_token,
    // all of them return with the first not-parsed token in m_token
    // if they are called, the term they parse is present, either they throw an error or are succesfull.
    // NOTE: the functions must check m_token unless they are absolutely sure it has been checked by the caller.
    // es: ParseVar() gets called if the keyword 'var' is in m_token and a var declaration is present for sure.
    void                    ParseFile(AstFile **);
    void                    ParseDependency(AstFile *file);
    void                    ParseDeclaration(AstFile *file);
    VarDeclaration          *ParseVar(void);
    ConstDeclaration        *ParseConst(void);
    TypeDeclaration         *ParseType(void);
    FuncDeclaration         *ParseFunctionDeclaration(void);
    IAstNode                *ParseTypeSpecification(void);
    AstArrayOrMatrixType    *ParseIndices(bool ismatrix);
    IAstNode                *ParseIniter(void);
    AstFuncType             *ParseFunctionType(void);
    void                    ParseArgsDef(AstFuncType *desc);
    AstArgumentDecl         *ParseSingleArgDef(void);
    AstBlock                *ParseBlock(void);
    IAstNode                *ParseLeftTermStatement(void);
    IAstNode                *ParseExpression(int max_priority = Lexer::max_priority + 1);
    IAstNode                *ParsePrefixExpression(void);
    IAstNode                *ParseLeftTerm(const char *errmess = NULL);
    void                    ParseRangesOrIndices(AstIndexing *node);
    void                    ParseArguments(AstFunCall *node);
    AstWhile                *ParseWhile(void);
    AstIf                   *ParseIf(void);
    AstFor                  *ParseFor(void);
public:
    Parser();
    ~Parser();

    void Init(Lexer *lexer);
    AstFile *ParseAll(void);
    const char *GetError(int index) { return(errors_.GetName(index)); }
};

}

#endif
