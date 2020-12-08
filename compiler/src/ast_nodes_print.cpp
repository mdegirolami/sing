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

void AstNodesPrint::ClosingBrace(void)
{
    if (indent_ > 0) {
        --indent_;
    }
    PrintIndent();
    fprintf(fd_, "}");
}

void AstNodesPrint::PrintFuncType(AstFuncType *node)
{
    PrintIndent();
    fprintf(fd_, "Function Type is%s pure has%s varargs has %d args {",
        node->ispure_ ? "" : " not", node->varargs_ ? "" : " not", node->arguments_.size());
    ++indent_;
    for (int ii = 0; ii < (int)node->arguments_.size(); ++ii) {
        if (node->arguments_[ii] != NULL) {
            PrintVarDeclaration(node->arguments_[ii]);
        }
    }
    PrintHierarchy(node->return_type_);
    ClosingBrace();
}

void AstNodesPrint::PrintPointerType(AstPointerType *node)
{
    PrintIndent();
    fprintf(fd_, "Pointer %s %s {", node->isconst_ ? "is const" : "", node->isweak_ ? "is weak" : "");
    ++indent_;
    PrintHierarchy(node->pointed_type_);
    ClosingBrace();
}

void AstNodesPrint::PrintMapType(AstMapType *node)
{
    PrintIndent();
    fprintf(fd_, "Map {");
    ++indent_;
    PrintHierarchy(node->key_type_);
    PrintHierarchy(node->returned_type_);
    ClosingBrace();
}

void AstNodesPrint::PrintArrayType(AstArrayType *node)
{
    PrintIndent();
    fprintf(fd_, "Array %s%s{", node->is_dynamic_ ? "is dynamic " : "", node->is_regular ? "is regular " : "");
    ++indent_;
    PrintHierarchy(node->expression_);
    PrintHierarchy(node->element_type_);
    ClosingBrace();
}

void AstNodesPrint::PrintNamedType(AstNamedType *node)
{
    PrintIndent();
    fprintf(fd_, "Type of name %s", node->name_.c_str());
    ++indent_;
    PrintHierarchy(node->next_component);
    --indent_;
}

void AstNodesPrint::PrintBaseType(AstBaseType *node)
{
    PrintIndent();
    fprintf(fd_, "%s type", Lexer::GetTokenString(node->base_type_));
}

void AstNodesPrint::PrintExpressionLeaf(AstExpressionLeaf *node)
{
    PrintIndent();
    switch (node->subtype_) {
    case TOKEN_INT32:
    case TOKEN_UINT32:
    case TOKEN_INT64:
    case TOKEN_UINT64:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
        fprintf(fd_, "%s %s %s%s", Lexer::GetTokenString(node->subtype_), 
            node->real_is_int_ ? "int" : "",
            node->real_is_negated_ ? "-" : "",
            node->value_.c_str());
        break;
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        fprintf(fd_, "%s %s %s%s %s%s", Lexer::GetTokenString(node->subtype_),
            node->real_is_int_ ? "int" : "",
            node->real_is_negated_ ? "-" : "",
            node->value_.c_str(),
            node->img_is_negated_ ? "-" : "",
            node->img_value_.c_str()
        );
        break;
    default:
        fprintf(fd_, "%s %s", Lexer::GetTokenString(node->subtype_), node->value_.c_str());
        break;
    }
}

void AstNodesPrint::PrintUnop(AstUnop *node)
{
    PrintIndent();
    fprintf(fd_, "Unop %s Operation {", Lexer::GetTokenString(node->subtype_));
    ++indent_;
    PrintHierarchy(node->operand_);
    ClosingBrace();
}

void AstNodesPrint::PrintBinop(AstBinop *node)
{
    PrintIndent();
    fprintf(fd_, "Binop %s Operation {", Lexer::GetTokenString(node->subtype_));
    ++indent_;
    PrintHierarchy(node->operand_left_);
    PrintHierarchy(node->operand_right_);
    ClosingBrace();
}

void AstNodesPrint::PrintFunCall(AstFunCall *node)
{
    PrintIndent();
    fprintf(fd_, "Function Call {");
    ++indent_;
    for (int ii = 0; ii < (int)node->arguments_.size(); ++ii) {
        if (node->arguments_[ii] != NULL) {
            PrintArgument(node->arguments_[ii]);
        }
    }
    PrintHierarchy(node->left_term_);
    ClosingBrace();
}

void AstNodesPrint::PrintArgument(AstArgument *node)
{
    PrintIndent();
    fprintf(fd_, "Argument %s {", node->name_.c_str());
    ++indent_;
    PrintHierarchy(node->expression_);
    ClosingBrace();
}

void AstNodesPrint::PrintIndexing(AstIndexing *node)
{
    PrintIndent();
    fprintf(fd_, "Indexing {");
    ++indent_;
    if (node->lower_value_ != NULL) {
        PrintHierarchy(node->lower_value_);
    } else {
        PrintIndent();
        fprintf(fd_, "Missing lower value");
    }
    if (node->upper_value_ != NULL) {
        PrintHierarchy(node->upper_value_);
    } else {
        PrintIndent();
        fprintf(fd_, "Missing upper value");
    }
    PrintHierarchy(node->indexed_term_);
    ClosingBrace();
}

void AstNodesPrint::PrintIncDec(AstIncDec *node)
{
    PrintIndent();
    fprintf(fd_, "%s Operation {", Lexer::GetTokenString(node->operation_));
    ++indent_;
    PrintHierarchy(node->left_term_);
    ClosingBrace();
}

void AstNodesPrint::PrintUpdate(AstUpdate *node)
{
    PrintIndent();
    fprintf(fd_, "Update with %s {", Lexer::GetTokenString(node->operation_));
    ++indent_;
    PrintHierarchy(node->left_term_);
    PrintHierarchy(node->right_term_);
    ClosingBrace();
}

void AstNodesPrint::PrintWhile(AstWhile *node)
{
    PrintIndent();
    fprintf(fd_, "While {");
    ++indent_;
    PrintHierarchy(node->expression_);
    PrintHierarchy(node->block_);
    ClosingBrace();
}

void AstNodesPrint::PrintIf(AstIf *node)
{
    PrintIndent();
    fprintf(fd_, "If {");
    ++indent_;
    for (int ii = 0; ii < (int)node->blocks_.size(); ++ii) {
        PrintHierarchy(node->expressions_[ii]);
        PrintHierarchy(node->blocks_[ii]);
    }
    PrintHierarchy(node->default_block_);
    ClosingBrace();
}

void AstNodesPrint::PrintFor(AstFor *node)
{
    PrintIndent();
    fprintf(fd_, "For {");
    ++indent_;
    if (node->index_ != NULL) {
        PrintVarDeclaration(node->index_);
    }
    if (node->iterator_ != NULL) {
        PrintVarDeclaration(node->iterator_);
    }
    if (node->set_ != NULL) {
        PrintHierarchy(node->set_);
    } else if (node->low_ != NULL && node->high_ != NULL) {
        PrintHierarchy(node->low_);
        PrintHierarchy(node->high_);
        if (node->step_ != NULL) {
            PrintHierarchy(node->step_);
        }
    }
    PrintHierarchy(node->block_);
    ClosingBrace();
}

void AstNodesPrint::PrintSwitch(AstSwitch *node)
{
    PrintIndent();
    fprintf(fd_, "Switch cases = %d {", node->case_values_.size());
    ++indent_;
    PrintHierarchy(node->switch_value_);
    int cases = 0;
    for (int ii = 0; ii < (int)node->statements_.size(); ++ii) {
        if (cases == node->statement_top_case_[ii]) {
            PrintIndent();
            fprintf(fd_, "default case:");
        } else {
            while (cases < node->statement_top_case_[ii]) {
                if (node->case_values_[cases] != nullptr) {
                    PrintHierarchy(node->case_values_[cases]);
                }
                ++cases;
            }
        }
        if (node->statements_[ii] == nullptr) {
            PrintIndent();
            fprintf(fd_, "empty");
        } else {
            PrintHierarchy(node->statements_[ii]);
        }
    }
    ClosingBrace();
}

void AstNodesPrint::PrintTypeSwitch(AstTypeSwitch *node)
{
    PrintIndent();
    fprintf(fd_, "TypeSwitch cases = %d {", node->case_types_.size());
    ++indent_;
    PrintHierarchy(node->reference_);
    PrintHierarchy(node->expression_);
    for (int ii = 0; ii < (int)node->case_types_.size(); ++ii) {
        if (node->case_types_[ii] == nullptr) {
            PrintIndent();
            fprintf(fd_, "else case:");
        } else {
            PrintHierarchy(node->case_types_[ii]);
        }
        PrintHierarchy(node->case_statements_[ii]);
    }
    ClosingBrace();
}

void AstNodesPrint::PrintSimpleStatement(AstSimpleStatement *node)
{
    PrintIndent();
    fprintf(fd_, "Statement %s", Lexer::GetTokenString(node->subtype_));
}

void AstNodesPrint::PrintReturn(AstReturn *node)
{
    PrintIndent();
    fprintf(fd_, "Return {");
    ++indent_;
    PrintHierarchy(node->retvalue_);
    ClosingBrace();
}

void AstNodesPrint::PrintBlock(AstBlock *node)
{
    PrintIndent();
    fprintf(fd_, "Block {");
    ++indent_;
    for (int ii = 0; ii < (int)node->block_items_.size(); ++ii) {
        PrintHierarchy(node->block_items_[ii]);
    }
    ClosingBrace();
}
 
void AstNodesPrint::PrintIniter(AstIniter *node)
{
    PrintIndent();
    fprintf(fd_, "Initer {");
    ++indent_;
    for (int ii = 0; ii < (int)node->elements_.size(); ++ii) {
        PrintHierarchy(node->elements_[ii]);
    }
    ClosingBrace();
}

void AstNodesPrint::PrintVarDeclaration(VarDeclaration *node)
{
    PrintIndent();
    fprintf(fd_, "Var %s ", node->name_.c_str());
    if (node->HasOneOfFlags(VF_READONLY)) fprintf(fd_, "read only ");
    if (node->HasOneOfFlags(VF_WRITEONLY)) fprintf(fd_, "write only ");
    if (node->HasOneOfFlags(VF_WASREAD)) fprintf(fd_, "was read ");
    if (node->HasOneOfFlags(VF_WASWRITTEN)) fprintf(fd_, "was written ");
    if (node->HasOneOfFlags(VF_ISARG)) fprintf(fd_, "is arg ");
    if (node->HasOneOfFlags(VF_ISFORINDEX)) fprintf(fd_, "is for index ");
    if (node->HasOneOfFlags(VF_ISFORITERATOR)) fprintf(fd_, "is for iterator ");
    if (node->HasOneOfFlags(VF_ISPOINTED)) fprintf(fd_, "is pointed ");
    if (node->initer_ != NULL) fprintf(fd_, "with initer ");
    fprintf(fd_, "{");
    ++indent_;
    PrintHierarchy(node->type_spec_);
    PrintHierarchy(node->initer_);
    ClosingBrace();
}

void AstNodesPrint::PrintTypeDeclaration(TypeDeclaration *node)
{
    PrintIndent();
    fprintf(fd_, "Type %s {", node->name_.c_str());
    ++indent_;
    PrintHierarchy(node->type_spec_);
    ClosingBrace();
}

void AstNodesPrint::PrintFuncDeclaration(FuncDeclaration *node)
{
    PrintIndent();
    if (node->is_class_member_) {
        fprintf(fd_, "Func %s.%s%s {", node->classname_.c_str(), node->name_.c_str(), node->is_muting_ ? " muting" : "");
    } else {
        fprintf(fd_, "Func %s%s {", node->name_.c_str(), node->is_muting_ ? " muting" : "");
    }
    ++indent_;
    PrintHierarchy(node->function_type_);
    PrintHierarchy(node->block_);
    ClosingBrace();
}

void AstNodesPrint::PrintEnumType(AstEnumType *node)
{
    PrintIndent();
    fprintf(fd_, "Enumeration {");
    ++indent_;
    for (int ii = 0; ii < (int)node->items_.size(); ++ii) {
        PrintIndent();
        fprintf(fd_, "case = %s", node->items_[ii].c_str());
        PrintHierarchy(node->initers_[ii]);
    }
    ClosingBrace();
}

void AstNodesPrint::PrintInterfaceType(AstInterfaceType *node)
{
    PrintIndent();
    fprintf(fd_, "Interface, members = %d, ancestors = %d {", node->members_.size(), node->ancestors_.size());
    ++indent_;
    for (int ii = 0; ii < (int)node->members_.size(); ++ii) {
        PrintHierarchy(node->members_[ii]);
    }
    for (int ii = 0; ii < (int)node->ancestors_.size(); ++ii) {
        PrintHierarchy(node->ancestors_[ii]);
    }
    ClosingBrace();
}

void AstNodesPrint::PrintClassType(AstClassType *node)
{
    PrintIndent();
    fprintf(fd_, "Class, members(var/fn/if) = %d, %d, %d {", node->member_vars_.size(), node->member_functions_.size(), node->member_interfaces_.size());
    ++indent_;
    for (int ii = 0; ii < (int)node->member_vars_.size(); ++ii) {
        PrintHierarchy(node->member_vars_[ii]);
    }
    for (int ii = 0; ii < (int)node->member_functions_.size(); ++ii) {
        PrintHierarchy(node->member_functions_[ii]);
        if (node->fn_implementors_[ii].length() > 0) {
            //PrintIndent();
            fprintf(fd_, " implemented by: %s", node->fn_implementors_[ii].c_str());
        }
    }
    for (int ii = 0; ii < (int)node->member_interfaces_.size(); ++ii) {
        PrintHierarchy(node->member_interfaces_[ii]);
        if (node->if_implementors_[ii].length() > 0) {
            //PrintIndent();
            fprintf(fd_, " implemented by: %s", node->if_implementors_[ii].c_str());
        }
    }
    ClosingBrace();
}

void AstNodesPrint::PrintDependency(AstDependency *node)
{
    fprintf(fd_, "\nRequired %s, %s", node->package_dir_.c_str(), node->package_name_.c_str());
}

void AstNodesPrint::PrintFile(const AstFile *node)
{
    int ii;

    fprintf(fd_, "\nFile with namespace = %s", node->namespace_.c_str());
    for (ii = 0; ii < (int)node->dependencies_.size(); ++ii) {
        PrintDependency(node->dependencies_[ii]);
    }
    for (ii = 0; ii < (int)node->declarations_.size(); ++ii) {
        PrintHierarchy(node->declarations_[ii]);
    }
}

void AstNodesPrint::PrintHierarchy(IAstNode *node)
{
    if (node == NULL) return;
    switch (node->GetType()) {
    case ANT_FUNC_TYPE:         PrintFuncType         ((AstFuncType          *)node);break;
    case ANT_POINTER_TYPE:      PrintPointerType      ((AstPointerType       *)node);break;
    case ANT_MAP_TYPE:          PrintMapType          ((AstMapType           *)node);break;
    case ANT_ARRAY_TYPE:        PrintArrayType        ((AstArrayType         *)node);break;
    case ANT_NAMED_TYPE:        PrintNamedType        ((AstNamedType         *)node);break;
    case ANT_BASE_TYPE:         PrintBaseType         ((AstBaseType          *)node);break;
    case ANT_ENUM_TYPE:         PrintEnumType         ((AstEnumType          *)node);break;
    case ANT_INTERFACE_TYPE:    PrintInterfaceType    ((AstInterfaceType     *)node);break;
    case ANT_CLASS_TYPE:        PrintClassType        ((AstClassType         *)node);break;
    case ANT_EXP_LEAF:          PrintExpressionLeaf   ((AstExpressionLeaf    *)node);break;
    case ANT_UNOP:              PrintUnop             ((AstUnop              *)node);break;
    case ANT_BINOP:             PrintBinop            ((AstBinop             *)node);break;
    case ANT_FUNCALL:           PrintFunCall          ((AstFunCall           *)node);break;
    case ANT_ARGUMENT:          PrintArgument         ((AstArgument          *)node);break;
    case ANT_INDEXING:          PrintIndexing         ((AstIndexing          *)node);break;
    case ANT_INCDEC:            PrintIncDec           ((AstIncDec            *)node);break;
    case ANT_UPDATE:            PrintUpdate           ((AstUpdate            *)node);break;
    case ANT_WHILE:             PrintWhile            ((AstWhile             *)node);break;
    case ANT_IF:                PrintIf               ((AstIf                *)node);break;
    case ANT_FOR:               PrintFor              ((AstFor               *)node);break;
    case ANT_SWITCH:            PrintSwitch           ((AstSwitch            *)node);break;
    case ANT_TYPESWITCH:        PrintTypeSwitch       ((AstTypeSwitch        *)node);break;
    case ANT_SIMPLE:            PrintSimpleStatement  ((AstSimpleStatement   *)node);break;
    case ANT_RETURN:            PrintReturn           ((AstReturn            *)node);break;
    case ANT_BLOCK:             PrintBlock            ((AstBlock             *)node);break;
    case ANT_INITER:            PrintIniter           ((AstIniter            *)node);break;
    case ANT_VAR:               PrintVarDeclaration   ((VarDeclaration       *)node);break;
    case ANT_TYPE:              PrintTypeDeclaration  ((TypeDeclaration      *)node);break;
    case ANT_FUNC:              PrintFuncDeclaration  ((FuncDeclaration      *)node);break;
    case ANT_DEPENDENCY:        PrintDependency       ((AstDependency        *)node);break;
    }
}

} // namespace