#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast_nodes.h"
#include "NamesList.h"
#include "helpers.h"

namespace SingNames {

// support for completion hints.
// doesn't return hints but identifies the elements at the left of the . or ( operator.
enum class CompletionType {
    NOT_FOUND, 
    FUNCTION,   // tag is the name of the class at the left of '.'
    TAG,        // tag is the alias of an extern file at the left of '.'
    OP          // node is the expression at the left of '.' or '('
};

struct CompletionHint {

    // inputs
    char    trigger;    // currently '.', '('
    int     row;
    int     col;

    // output
    CompletionType  type;
    string          tag;
    IAstExpNode     *node;

    CompletionHint() { node = nullptr; type = CompletionType::NOT_FOUND; }
};

enum class ParseMode { 
    FULL,
    FOR_REFERENCE,      // skip local definitions, skip functions body, skip comments assignments
    INTELLISENSE        // allows functions declarations without a body (prototypes)
};

class Parser {
    Lexer           *m_lexer;
    Token           m_token;
    CompletionHint  *completion_;
    bool            insert_completion_node_;
    int32_t         package_idx_;

    bool        on_error_;      // needs to recover skipping some stuff
    bool        has_errors_;    // found in previous blocks/declarations.
    ErrorList   *errors_;       // not owning !!
    AstFile     *root_;
    int         curly_indent_;  // need it for error recovery
    vector<int> remarkable_lines;   // you can associate a remark to these

    void    CheckSemicolon(void);
    bool    Advance(void);
    void    Error(const char *message);                         // throws and need to recover
    void    SetError(const char *message, int row, int column, int endrow, int endcol); // just adds to the errors list
    bool    SkipToNextStatement(int level);                     // returns false if exits from the function/block
    void    SkipToNextDeclaration(void);
    bool    OnDeclarationToken(void);
    bool    OutOfFunctionToken(void);
    Token   SkipToken(void);
    void    RecordPosition(IAstNode *node, bool save_as_remarkable = true);
    void    SaveRemarkableRow(int row);
    void    FillPositionInfo(PositionInfo *pnfo);
    void    UpdateEndPosition(IAstNode *node);
    void    CheckCommentsAssignments(void);
    bool    CommentLineIsAllowed(int line, int *scan);

    // these functions gets called with the first token of the parsed term already in m_token,
    // all of them return with the first not-parsed token in m_token
    // if they are called, the term they parse is present, either they throw an error or are succesfull.
    // NOTE: the functions must check m_token unless they are absolutely sure it has been checked by the caller.
    // es: ParseVar() gets called if the keyword 'var' is in m_token and a var declaration is present for sure.
    void                    ParseFile(AstFile *file, ParseMode mode);
    void                    ParseNamespace(AstFile *file);
    void                    ParseDependency(AstFile *file);
    void                    ParseDeclaration(AstFile *file, ParseMode mode);
    VarDeclaration          *ParseVar(void);
    VarDeclaration          *ParseConst(void);
    TypeDeclaration         *ParseType(void);
    FuncDeclaration         *ParseFunctionDeclaration(ParseMode mode);
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
    IAstExpNode             *ParseExpression(void);
    IAstExpNode             *ParsePrefixExpression(const char *errmess);
    IAstExpNode             *ParsePostfixExpression(const char *errmess);
    IAstExpNode             *ParseExpressionTerm(const char *errmess);
    AstIndexing             *ParseRangesOrIndices(IAstExpNode *indexed);
    void                    ParseArguments(AstFunCall *node);
    AstWhile                *ParseWhile(void);
    AstIf                   *ParseIf(void);
    AstFor                  *ParseFor(void);
    AstSwitch               *ParseSwitch(void);
    AstTypeSwitch           *ParseTypeSwitch(void);
    IAstExpNode             *CheckForCastedLiterals(AstUnop *node);
    AstExpressionLeaf       *GetLiteralRoot(IAstExpNode *node, bool *negative);
    bool                    OnCompletionHint(void);
public:
    Parser();
    ~Parser();

    // note: completion may be null
    void Init(Lexer *lexer, CompletionHint *completion, int32_t package_idx);
    AstFile *ParseAll(ErrorList *errors, ParseMode mode);
};

}

#endif
