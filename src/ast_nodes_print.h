#ifndef AST_NODES_PRINT_H
#define AST_NODES_PRINT_H

#include "ast_nodes.h"

namespace SingNames {

class AstNodesPrint {
    FILE    *fd_;
    int     indent_;

    void PrintIndent(void);
    void ClosingBrace(void);

    void PrintFuncType          (AstFuncType          *node);
    void PrintPointerType       (AstPointerType       *node);
    void PrintMapType           (AstMapType           *node);
    void PrintArrayType         (AstArrayType         *node);
    void PrintNamedType         (AstNamedType         *node);
    void PrintBaseType          (AstBaseType          *node);
    void PrintEnumType          (AstEnumType          *node);
    void PrintInterfaceType     (AstInterfaceType     *node);
    void PrintClassType         (AstClassType         *node);
    void PrintExpressionLeaf    (AstExpressionLeaf    *node);
    void PrintUnop              (AstUnop              *node);
    void PrintBinop             (AstBinop             *node);
    void PrintFunCall           (AstFunCall           *node);
    void PrintArgument          (AstArgument          *node);
    void PrintIndexing          (AstIndexing          *node);
    void PrintIncDec            (AstIncDec            *node);
    void PrintUpdate            (AstUpdate            *node);
    void PrintWhile             (AstWhile             *node);
    void PrintIf                (AstIf                *node);
    void PrintFor               (AstFor               *node);
    void PrintSwitch            (AstSwitch            *node);
    void PrintTypeSwitch        (AstTypeSwitch        *node);
    void PrintSimpleStatement   (AstSimpleStatement   *node);
    void PrintReturn            (AstReturn            *node);
    void PrintBlock             (AstBlock             *node);
    void PrintIniter            (AstIniter            *node);
    void PrintVarDeclaration    (VarDeclaration       *node);
    void PrintTypeDeclaration   (TypeDeclaration      *node);
    void PrintFuncDeclaration   (FuncDeclaration      *node);
    void PrintDependency        (AstDependency        *node);

    void PrintHierarchy(IAstNode *node);

public:
    void Init(FILE *fd) { fd_ = fd; indent_ = 0; }
    void PrintFile(AstFile *node);
};

} // namespace

#endif