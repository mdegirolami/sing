#ifndef TEST_VISITOR_H
#define TEST_VISITOR_H

#include "ast_nodes.h"

namespace SingNames {

class TestVisitor : public IAstVisitor {
    FILE    *fd_;
    int     indent_;
    Lexer   *lexer_;

    void PrintIndent(void);
    void CloseBraces(void);
public:
    void Init(FILE *fd, Lexer *lexer) {
        fd_ = fd;
        indent_ = 0;
        lexer_ = lexer;
    }

    // base structure
    virtual void File(const char *package);
    virtual void PackageRef(const char *path, const char *package_name);
    virtual void BeginVarDeclaration(const char *name, bool isvolatile, bool has_initer);
    virtual void EndVarDeclaration(const char *name, bool isvolatile, bool has_initer) { CloseBraces(); }
    virtual void BeginConstDeclaration(const char *name);
    virtual void EndConstDeclaration(const char *name) { CloseBraces(); }
    virtual void BeginTypeDeclaration(const char *name);
    virtual void EndTypeDeclaration(const char *name) { CloseBraces(); }
    virtual void BeginFuncDeclaration(const char *name, bool ismember, const char *classname);
    virtual void EndFuncDeclaration(const char *name, bool ismember, const char *classname) { CloseBraces(); }
    virtual void BeginIniter(void);
    virtual void EndIniter(void) { CloseBraces(); }

    // statements
    virtual void BeginBlock(void);
    virtual void EndBlock(void) { CloseBraces(); }
    virtual void BeginAssignments(int num_assegnee);
    virtual void EndAssignments(int num_assegnee) { CloseBraces(); }
    virtual void BeginUpdateStatement(Token type);
    virtual void EndUpdateStatement(Token type) { CloseBraces(); }
    virtual void BeginLeftTerm(int index);
    virtual void BeginRightTerm(int index);
    virtual void BeginIncDec(Token type);
    virtual void EndIncDec(Token type) { CloseBraces(); }
    virtual void BeginWhile(void);
    virtual void EndWhile(void) { CloseBraces(); }
    virtual void BeginIf(void);
    virtual void EndIf(void) { CloseBraces(); }
    virtual void BeginIfClause(int num);
    virtual void EndIfClause(int num) { CloseBraces(); }
    virtual void BeginFor(const char *index, const char *iterator);
    virtual void EndFor(const char *index, const char *iterator) { CloseBraces(); }
    virtual void BeginForSet(void);
    virtual void EndForSet(void) { CloseBraces(); }
    virtual void BeginForLow(void);
    virtual void EndForLow(void) { CloseBraces(); }
    virtual void BeginForHigh(void);
    virtual void EndForHigh(void) { CloseBraces(); }
    virtual void BeginForStep(void);
    virtual void EndForStep(void) { CloseBraces(); }
    virtual void SimpleStatement(Token token);
    virtual void BeginReturn(void);
    virtual void EndReturn(void) { CloseBraces(); }

    // expressions
    virtual void ExpLeaf(Token type, const char *value);
    virtual void BeginUnop(Token subtype);
    virtual void EndUnop(Token subtype) { CloseBraces(); }
    virtual void BeginBinop(Token subtype);
    virtual void BeginBinopSecondArg(void);
    virtual void EndBinop(Token subtype) { CloseBraces(); }
    virtual void BeginFunCall(void);
    virtual void EndFunCall(void) { CloseBraces(); }
    virtual void FunCallArg(int num);
    virtual void BeginArgument(const char *name);
    virtual void EndArgument(const char *name) { CloseBraces(); }
    virtual void CastTypeBegin(void);
    virtual void CastTypeEnd(void) { CloseBraces(); }
    virtual void BeginIndexing(void);
    virtual void EndIndexing(void) { CloseBraces(); }
    virtual void Index(int num, bool has_lower_bound, bool has_upper_bound);

    // types
    virtual void BeginFuncType(bool ispure_, bool varargs_, int num_args);
    virtual void EndFuncType(bool ispure_, bool varargs_, int num_args) { CloseBraces(); }
    virtual void BeginArgumentDecl(Token direction, const char *name, bool has_initer);
    virtual void EndArgumentDecl(Token direction, const char *name, bool has_initer) { CloseBraces(); }
    virtual void BeginArrayOrMatrixType(bool is_matrix_, int dimensions_count);
    virtual void EndArrayOrMatrixType(bool is_matrix_, int dimensions_count) { CloseBraces(); }
    virtual void ConstIntExpressionValue(int value) {}
    virtual void BeginMapType(void);
    virtual void MapReturnType(void);
    virtual void EndMapType(void) { CloseBraces(); }
    virtual void BeginPointerType(bool isconst, bool isweak);
    virtual void EndPointerType(bool isconst, bool isweak) { CloseBraces(); }
    virtual void NameOfType(const char *name, int component_index); // package may be NULL
    virtual void BaseType(Token token);
};

}   // namespace
    
#endif
