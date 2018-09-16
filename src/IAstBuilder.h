#ifndef IAST_BUILDER_H
#define IAST_BUILDER_H

#include "lexer.h"

namespace StayNames {

class IAstBuilder {
public:

    // general
    virtual void BeginFile(const char *package_name) = 0;
    virtual void EndFile(void) = 0;
    virtual void Dependency(const char *package_path, const char *local_name) = 0;

    // declarations
    virtual void BeginDecl(Token decl_type, bool is_member) = 0;
    virtual void SetClassName(const char *classname) = 0;
    virtual void SetName(const char *name) = 0;
    virtual void SetExternName(const char *package_name, const char *name) = 0;
    virtual void SetVolatile(void) = 0;
    virtual void BeginIniter(void) = 0;
    virtual void EndIniter(void) = 0;

    // types
    virtual void SetBaseType(Token basetype) = 0;
    virtual void SetNamedType(const char *name) = 0;
    virtual void SetExternType(const char *package_name, const char *type_name) = 0;
    virtual void SetArrayType(int num_elements) = 0;
    virtual void SetMatrixType(int num_elements) = 0;
    virtual void AddADimension(int num_elements) = 0;
    virtual void SetMapType(void) = 0;
    virtual void SetPointerType(bool isconst, bool isweak) = 0;
    virtual void SetFunctionType(bool ispure) = 0;
    virtual void SetArgDeclaration(Token direction, const char *name) = 0;

    // blocks
    virtual void BeginBlock(void) = 0;
    virtual void EndBlock(void) = 0;
    virtual void AssignStatement(void) = 0;
    virtual void UpdateStatement(void) = 0;
    virtual void IncrementStatement(Token inc_or_dec) = 0;
    virtual void EndOfStatement(void) = 0;

    // expressions
    virtual void BeginExpression(void) = 0;
    virtual void SetExpLeaf(Token subtype, const char *value) = 0;
    virtual void SetUnary(Token unop) = 0;
    virtual void SetBinary(Token binop) = 0;
    virtual void OpenBraces(void) = 0;
    virtual void CloseBraces(void) = 0;
    virtual void BeginFunctionArgs(void) = 0;
    virtual void AddArgument(const char *name) = 0;
    virtual void ReinterpretCast(void) = 0;
    virtual void EndFunctionArgs(void) = 0;
    virtual void BeginIndices(void) = 0;
    virtual void StartIndex(void) = 0;
    virtual void EndIndices(void) = 0;
};

}

#endif
