#include "ast_nodes_print.h"

#define FORALL()

namespace SingNames {

void AstNodesPrint::PrintIndent(void)
{
    static const char *indent = "    ";
    fprintf(fd_, "\n");
    for (int ii = 0; ii < indent_; ++ii) {
        fprintf(fd_, indent);
    }
}

void AstNodesPrint::CloseBraces(void)
{
    if (indent_ > 0) {
        --indent_;
    }
    PrintIndent();
    fprintf(fd_, "}");
}

void AstNodesPrint::PrintArgumentDecl(AstArgumentDecl *node)
{
}

void AstNodesPrint::PrintFuncType(AstFuncType *node)
{
}

void AstNodesPrint::PrintPointerType(AstPointerType *node)
{
}

void AstNodesPrint::PrintMapType(AstMapType *node)
{
}

void AstNodesPrint::PrintArrayOrMatrixType(AstArrayOrMatrixType *node)
{
}

void AstNodesPrint::PrintQualifiedType(AstQualifiedType *node)
{
}

void AstNodesPrint::PrintNamedType(AstNamedType *node)
{
}

void AstNodesPrint::PrintBaseType(AstBaseType *node)
{
}

void AstNodesPrint::PrintExpressionLeaf(AstExpressionLeaf *node)
{
}

void AstNodesPrint::PrintUnop(AstUnop *node)
{
}

void AstNodesPrint::PrintBinop(AstBinop *node)
{
}

void AstNodesPrint::PrintFunCall(AstFunCall *node)
{
}

void AstNodesPrint::PrintArgument(AstArgument *node)
{
}

void AstNodesPrint::PrintIndexing(AstIndexing *node)
{
}

void AstNodesPrint::PrintIncDec(AstIncDec *node)
{
}

void AstNodesPrint::PrintUpdate(AstUpdate *node)
{
}

void AstNodesPrint::PrintAssignment(AstAssignment *node)
{
}

void AstNodesPrint::PrintWhile(AstWhile *node)
{
}

void AstNodesPrint::PrintIf(AstIf *node)
{
}

void AstNodesPrint::PrintFor(AstFor *node)
{
}

void AstNodesPrint::PrintSimpleStatement(AstSimpleStatement *node)
{
}

void AstNodesPrint::PrintReturn(AstReturn *node)
{
}

void AstNodesPrint::PrintBlock(AstBlock *node)
{
}
 
void AstNodesPrint::PrintIniter(AstIniter *node)
{
}

void AstNodesPrint::PrintVarDeclaration(VarDeclaration *node)
{
    PrintIndent();
    fprintf(fd_, "Var %s", name);
    if (isvolatile) fprintf(fd_, " volatile ");
    if (has_initer) fprintf(fd_, " with initer ");
    fprintf(fd_, "{");
    ++indent_;
}

void AstNodesPrint::PrintConstDeclaration(ConstDeclaration *node)
{
}

void AstNodesPrint::PrintTypeDeclaration(TypeDeclaration *node)
{
}

void AstNodesPrint::PrintFuncDeclaration(FuncDeclaration *node)
{
}

void AstNodesPrint::PrintDependency(AstDependency *node)
{
    fprintf(fd_, "\nRequired %s", node->package_dir_);
}

void AstNodesPrint::PrintFile(AstFile *node)
{
    IAstNode *decl;

    fprintf(fd_, "\nPackage = %s", node->package_name_.c_str());
    for (int ii = 0; ii < node->dependencies_.size(); ++ii) {
        PrintDependency(node->dependencies_[ii]);
    }
    for (int ii = 0; ii < node->declarations_.size(); ++ii) {
        decl = node->declarations_[ii];
        switch (decl->GetType()) {
        case ANT_VAR:
            PrintVarDeclaration((VarDeclaration*)decl);
            break;
        case ANT_FUNC:
            PrintFuncDeclaration((FuncDeclaration*)decl);
            break;
        }
    }
}

void AstNodesPrint::PrintHierarchy(IAstNode *node)
{
    switch (node->GetType()) {
    case ANT_ARGUMENT_DECLARE:  PrintArgumentDecl     ((AstArgumentDecl      *)node);break;
    case ANT_FUNC_TYPE:         PrintFuncType         ((AstFuncType          *)node);break;
    case ANT_POINTER_TYPE:      PrintPointerType      ((AstPointerType       *)node);break;
    case ANT_MAP_TYPE:          PrintMapType          ((AstMapType           *)node);break;
    case ANT_ARRAY_TYPE:        PrintArrayOrMatrixType((AstArrayOrMatrixType *)node);break;
    case ANT_QUALIFIED_TYPE:    PrintQualifiedType    ((AstQualifiedType     *)node);break;
    case ANT_NAMED_TYPE:        PrintNamedType        ((AstNamedType         *)node);break;
    case ANT_BASE_TYPE:         PrintBaseType         ((AstBaseType          *)node);break;
    case ANT_EXP_LEAF:          PrintExpressionLeaf   ((AstExpressionLeaf    *)node);break;
    case ANT_UNOP:              PrintUnop             ((AstUnop              *)node);break;
    case ANT_BINOP:             PrintBinop            ((AstBinop             *)node);break;
    case ANT_FUNCALL:           PrintFunCall          ((AstFunCall           *)node);break;
    case ANT_ARGUMENT:          PrintArgument         ((AstArgument          *)node);break;
    case ANT_INDEXING:          PrintIndexing         ((AstIndexing          *)node);break;
    case ANT_INCDEC:            PrintIncDec           ((AstIncDec            *)node);break;
    case ANT_UPDATE:            PrintUpdate           ((AstUpdate            *)node);break;
    case ANT_ASSIGNMENT:        PrintAssignment       ((AstAssignment        *)node);break;
    case ANT_WHILE:             PrintWhile            ((AstWhile             *)node);break;
    case ANT_IF:                PrintIf               ((AstIf                *)node);break;
    case ANT_FOR:               PrintFor              ((AstFor               *)node);break;
    case ANT_SIMPLE:            PrintSimpleStatement  ((AstSimpleStatement   *)node);break;
    case ANT_RETURN:            PrintReturn           ((AstReturn            *)node);break;
    case ANT_BLOCK:             PrintBlock            ((AstBlock             *)node);break;
    case ANT_INITER:            PrintIniter           ((AstIniter            *)node);break;
    case ANT_VAR:               PrintVarDeclaration   ((VarDeclaration       *)node);break;
    case ANT_CONST:             PrintConstDeclaration ((ConstDeclaration     *)node);break;
    case ANT_TYPE:              PrintTypeDeclaration  ((TypeDeclaration      *)node);break;
    case ANT_FUNC:              PrintFuncDeclaration  ((FuncDeclaration      *)node);break;
    case ANT_DEPENDENCY:        PrintDependency       ((AstDependency        *)node);break;
    }
}

} // namespace