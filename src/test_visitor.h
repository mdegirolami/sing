#ifndef TEST_VISITOR_H
#define TEST_VISITOR_H

#include "ast_nodes.h"

namespace StayNames {

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
    virtual void BeginFuncDeclaration(const char *name, bool ismember, const char *classname);
    virtual void EndFuncDeclaration(const char *name, bool ismember, const char *classname) { CloseBraces(); }

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

    // expressions
    virtual void ExpLeaf(Token type, const char *value);
    virtual void BeginUnop(Token subtype);
    virtual void EndUnop(Token subtype) { CloseBraces(); }
    virtual void BeginBinop(Token subtype);
    virtual void BeginBinopSecondArg(void);
    virtual void EndBinop(Token subtype) { CloseBraces(); }

    // types
    virtual void BeginFuncType(bool ispure_, bool varargs_, int num_args);
    virtual void EndFuncType(bool ispure_, bool varargs_, int num_args) { CloseBraces(); }
    virtual void BeginArgumentDecl(Token direction, const char *name, bool has_initer);
    virtual void EndArgumentDecl(Token direction, const char *name, bool has_initer) { CloseBraces(); }
    virtual void BeginArrayOrMatrixType(bool is_matrix_, int dimensions_count);
    virtual void EndArrayOrMatrixType(bool is_matrix_, int dimensions_count) { CloseBraces(); }
    virtual void ConstIntExpressionValue(int value) {}
    virtual void NameOfType(const char *package, const char *name); // package may be NULL
    virtual void BaseType(Token token);
};

}   // namespace
    
#endif
