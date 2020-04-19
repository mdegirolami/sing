#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast_nodes.h"
#include "NamesList.h"
#include "helpers.h"

namespace SingNames {

class Parser {
    Lexer *m_lexer;
    Token m_token;

    bool        on_error_;      // needs to recover skipping some stuff
    bool        has_errors_;    // found in previous blocks/declarations.
    ErrorList   *errors_;       // not owning !!
    bool        for_reference_;
    AstFile     *root_;
    int         curly_indent_;  // need it for error recovery

    void    CheckSemicolon(void);
    bool    Advance(void);
    void    Error(const char *message);                         // throws and need to recover
    void    SetError(const char *message, int row, int column); // just adds to the errors list
    bool    SkipToNextStatement(int level);                     // returns false if exits from the function/block
    void    SkipToNextDeclaration(void);
    bool    OnDeclarationToken(void);
    bool    OutOfFunctionToken(void);
    Token   SkipToken(void);
    void    RecordPosition(IAstNode *node);
    void    FillPositionInfo(PositionInfo *pnfo);
    void    UpdateEndPosition(IAstNode *node);
    void    AttachCommentsToNodes(void);
    void    AssignCommentsToNode(IAstNode *node, int first, int count);

    // these functions gets called with the first token of the parsed term already in m_token,
    // all of them return with the first not-parsed token in m_token
    // if they are called, the term they parse is present, either they throw an error or are succesfull.
    // NOTE: the functions must check m_token unless they are absolutely sure it has been checked by the caller.
    // es: ParseVar() gets called if the keyword 'var' is in m_token and a var declaration is present for sure.
    void                    ParseFile(AstFile *file, bool for_reference);
    void                    ParseNamespace(AstFile *file);
    void                    ParseDependency(AstFile *file);
    void                    ParseDeclaration(AstFile *file, bool for_reference);
    VarDeclaration          *ParseVar(void);
    VarDeclaration          *ParseConst(void);
    TypeDeclaration         *ParseType(void);
    FuncDeclaration         *ParseFunctionDeclaration(bool skip_body);
    TypeDeclaration         *ParseEnum(void);
    TypeDeclaration         *ParseInterface(void);
    TypeDeclaration         *ParseClass(void);
    IAstTypeNode            *ParseTypeSpecification(void);
    AstNamedType            *ParseNamedType(void);
    AstArrayType            *ParseIndices(void);
    IAstNode                *ParseIniter(void);
    AstFuncType             *ParseFunctionType(bool is_body);
    void                    ParseArgsDef(AstFuncType *desc, bool is_function_body);
    VarDeclaration          *ParseSingleArgDef(bool is_function_body, bool *mandatory_initer);
    AstBlock                *ParseBlock(void);
    IAstNode                *ParseStatement(bool allow_let_and_var);
    IAstNode                *ParseLeftTermStatement(void);
    IAstExpNode             *ParseExpression(/*int max_priority = Lexer::max_priority + 1*/);
    IAstExpNode             *ParsePrefixExpression(void);
    IAstExpNode             *ParseLeftTerm(const char *errmess = NULL);
    AstIndexing             *ParseRangesOrIndices(IAstExpNode *indexed);
    void                    ParseArguments(AstFunCall *node);
    AstWhile                *ParseWhile(void);
    AstIf                   *ParseIf(void);
    AstFor                  *ParseFor(void);
    AstSwitch               *ParseSwitch(void);
    AstTypeSwitch           *ParseTypeSwitch(void);
    IAstExpNode             *CheckForCastedLiterals(AstUnop *node);
    AstExpressionLeaf       *GetLiteralRoot(IAstExpNode *node, bool *negative);
public:
    Parser();
    ~Parser();

    void Init(Lexer *lexer);
    AstFile *ParseAll(ErrorList *errors, bool for_reference);  // for_reference: only public declarations, skipping function bodies, skipping comments
};

}

#endif
