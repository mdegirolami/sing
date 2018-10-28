#include "test_visitor.h"

namespace SingNames {

void TestVisitor::PrintIndent(void)
{
    static const char *indent = "    ";
    fprintf(fd_, "\n");
    for (int ii = 0; ii < indent_; ++ii) {
        fprintf(fd_, indent);
    }
}

void TestVisitor::CloseBraces(void)
{
    if (indent_ > 0) {
        --indent_;
    }
    PrintIndent();
    fprintf(fd_, "}");
}

void TestVisitor::File(const char *package)
{
    fprintf(fd_, "\nPackage = %s", package);
}

void TestVisitor::PackageRef(const char *path, const char *package_name)
{
    fprintf(fd_, "\nRequired %s, named %s", path, package_name);
}

void TestVisitor::BeginVarDeclaration(const char *name, bool isvolatile, bool has_initer)
{
    PrintIndent();
    fprintf(fd_, "Var %s", name);
    if (isvolatile) fprintf(fd_, " volatile ");
    if (has_initer) fprintf(fd_, " with initer ");
    fprintf(fd_, "{");
    ++indent_;
}

void TestVisitor::BeginConstDeclaration(const char *name)
{
    PrintIndent();
    fprintf(fd_, "Const %s {", name);
    ++indent_;
}

void TestVisitor::BeginTypeDeclaration(const char *name)
{
    PrintIndent();
    fprintf(fd_, "Type %s {", name);
    ++indent_;
}

void TestVisitor::BeginFuncDeclaration(const char *name, bool ismember, const char *classname)
{
    PrintIndent();
    if (ismember) {
        fprintf(fd_, "Func %s.%s {", classname, name);
    } else {
        fprintf(fd_, "Func %s {", name);
    }
    ++indent_;
}

void TestVisitor::BeginIniter(void)
{
    PrintIndent();
    fprintf(fd_, "Initer {");
    ++indent_;
}

void TestVisitor::BeginBlock(void)
{
    PrintIndent();
    fprintf(fd_, "Block {");
    ++indent_;
}

void TestVisitor::BeginAssignments(int num_assignee)
{
    PrintIndent();
    fprintf(fd_, "Assignment with %d assignee {", num_assignee);
    ++indent_;
}

void TestVisitor::BeginUpdateStatement(Token type)
{
    PrintIndent();
    fprintf(fd_, "Update with %s {", lexer_->GetTokenString(type));
    ++indent_;
}

void TestVisitor::BeginLeftTerm(int index)
{
    PrintIndent();
    fprintf(fd_, "Left term #%d", index);
}

void TestVisitor::BeginRightTerm(int index)
{
    PrintIndent();
    fprintf(fd_, "Right term #%d", index);
}

void TestVisitor::BeginIncDec(Token type)
{
    PrintIndent();
    fprintf(fd_, "%s Operation {", lexer_->GetTokenString(type));
    ++indent_;
}

void TestVisitor::BeginWhile(void)
{
    PrintIndent();
    fprintf(fd_, "While {");
    ++indent_;
}

void TestVisitor::BeginIf(void)
{
    PrintIndent();
    fprintf(fd_, "If {");
    ++indent_;
}

void TestVisitor::BeginIfClause(int num)
{
    PrintIndent();
    fprintf(fd_, "If Clause %d {", num);
    ++indent_;
}

void TestVisitor::BeginFor(const char *index, const char *iterator)
{
    PrintIndent();
    fprintf(fd_, "For %s, %s {", index, iterator);
    ++indent_;
}

void TestVisitor::BeginForSet(void)
{
    PrintIndent();
    fprintf(fd_, "For Set Expression {");
    ++indent_;
}

void TestVisitor::BeginForLow(void)
{
    PrintIndent();
    fprintf(fd_, "For Low Expression {");
    ++indent_;
}

void TestVisitor::BeginForHigh(void)
{
    PrintIndent();
    fprintf(fd_, "For High Expression {");
    ++indent_;
}

void TestVisitor::BeginForStep(void)
{
    PrintIndent();
    fprintf(fd_, "For Step Expression {");
    ++indent_;
}

void TestVisitor::SimpleStatement(Token token)
{
    PrintIndent();
    fprintf(fd_, "Statement %s", lexer_->GetTokenString(token));
}

void TestVisitor::BeginReturn(void)
{
    PrintIndent();
    fprintf(fd_, "Return {");
    ++indent_;
}

void TestVisitor::ExpLeaf(Token type, const char *value)
{
    PrintIndent();
    fprintf(fd_, "%s %s", lexer_->GetTokenString(type), value);
}

void TestVisitor::BeginUnop(Token subtype)
{
    PrintIndent();
    fprintf(fd_, "Unop %s Operation {", lexer_->GetTokenString(subtype));
    ++indent_;
}

void TestVisitor::BeginBinop(Token subtype)
{
    PrintIndent();
    fprintf(fd_, "Binop %s Operation {", lexer_->GetTokenString(subtype));
    ++indent_;
}

void TestVisitor::BeginBinopSecondArg(void)
{
    PrintIndent();
    fprintf(fd_, "Binop second arg :");
}

void TestVisitor::BeginFunCall(void)
{
    PrintIndent();
    fprintf(fd_, "Function Call {");
    ++indent_;
}

void TestVisitor::FunCallArg(int num)
{
    PrintIndent();
    fprintf(fd_, "Arg %d", num);
}

void TestVisitor::BeginArgument(const char *name)
{
    PrintIndent();
    fprintf(fd_, "Argument %s {", name);
    ++indent_;
}

void TestVisitor::CastTypeBegin(void)
{
    PrintIndent();
    fprintf(fd_, "Cast Type {");
    ++indent_;
}

void TestVisitor::BeginIndexing(void)
{
    PrintIndent();
    fprintf(fd_, "Indexing {");
    ++indent_;
}

void TestVisitor::Index(int num, bool has_lower_bound, bool has_upper_bound)
{
    PrintIndent();
    fprintf(fd_, "Index %d %s %s", num, has_lower_bound ? "has lower" : "", has_upper_bound ? "has upper" : "");
}

void TestVisitor::BeginFuncType(bool ispure, bool varargs, int num_args)
{
    PrintIndent();
    fprintf(fd_, "Function Type is%s pure has%s varargs has %d args {",
        ispure ? "" : " not", varargs ? "" : " not", num_args);
    ++indent_;
}

void TestVisitor::BeginArgumentDecl(Token direction, const char *name, bool has_initer)
{
    PrintIndent();
    fprintf(fd_, "Argument ");
    if (direction == TOKEN_IN || direction == TOKEN_OUT || direction == TOKEN_IO) {
        fprintf(fd_, "%s ", lexer_->GetTokenString(direction));
    }
    if (has_initer) {
        fprintf(fd_, "has initer ");
    }
    fprintf(fd_, "is named %s {", name);
    ++indent_;
}

void TestVisitor::BeginArrayOrMatrixType(bool is_matrix_, int dimensions_count)
{
    PrintIndent();
    fprintf(fd_, "Array of %d dimensions is%s matrix {", dimensions_count, is_matrix_ ? "" : " not");
    ++indent_;
}

void TestVisitor::BeginMapType(void)
{
    PrintIndent();
    fprintf(fd_, "Map {");
    ++indent_;
}

void TestVisitor::MapReturnType(void)
{
    PrintIndent();
    fprintf(fd_, "Map Return Value :");
}

void TestVisitor::BeginPointerType(bool isconst, bool isweak)
{
    PrintIndent();
    fprintf(fd_, "Pointer %s %s {", isconst ? "is const" : "", isweak ? "is weak" : "");
    ++indent_;
}

void TestVisitor::NameOfType(const char *name, int component_index)
{
    if (component_index == 0) {
        PrintIndent();
        fprintf(fd_, "TypeName %s", name);
    } else {
        fprintf(fd_, ".%s", name);
    }
}

void TestVisitor::BaseType(Token token)
{
    PrintIndent();
    fprintf(fd_, "%s type", lexer_->GetTokenString(token));
}

} // namespace