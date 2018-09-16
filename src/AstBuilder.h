#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "IAstBuilder.h"
#include "ast_nodes.h"

namespace StayNames {

class AstBuilder : public IAstBuilder {
    AstFile             *root;
    vector<IAstNode*>   stack;
    IAstNode            **type_receiver;    // where we attach new type nodes

    void Error(const char *message) {}
public:

    // general
    virtual void BeginFile(const char *package_name);
    virtual void EndFile(void);
    virtual void Dependency(const char *package_path, const char *local_name);

    // declarations
    virtual void BeginDecl(Token decl_type, bool is_member);
    virtual void SetClassName(const char *classname);
    virtual void SetName(const char *name);
    virtual void SetExternName(const char *package_name, const char *name);
    virtual void SetVolatile(void);
    virtual void BeginIniter(void);
    virtual void EndIniter(void);

    // types
    virtual void SetBaseType(Token basetype);
    virtual void SetNamedType(const char *name);
    virtual void SetExternType(const char *package_name, const char *type_name);
    virtual void SetArrayType(int num_elements);
    virtual void SetMatrixType(int num_elements);
    virtual void AddADimension(int num_elements);
    virtual void SetMapType(void);
    virtual void SetPointerType(bool isconst, bool isweak);
    virtual void SetFunctionType(bool ispure);
    virtual void SetArgDeclaration(Token direction, const char *name);

    // blocks
    virtual void BeginBlock(void);
    virtual void EndBlock(void);
    virtual void AssignStatement(void);
    virtual void UpdateStatement(void);
    virtual void IncrementStatement(Token inc_or_dec);
    virtual void EndOfStatement(void);

    // expressions
    virtual void BeginExpression(void);
    virtual void SetExpLeaf(Token subtype, const char *value);
    virtual void SetUnary(Token unop);
    virtual void SetBinary(Token binop);
    virtual void OpenBraces(void);
    virtual void CloseBraces(void);
    virtual void BeginFunctionArgs(void);
    virtual void AddArgument(const char *name);
    virtual void ReinterpretCast(void);
    virtual void EndFunctionArgs(void);
    virtual void BeginIndices(void);
    virtual void StartIndex(void);
    virtual void EndIndices(void);
};

}

#endif