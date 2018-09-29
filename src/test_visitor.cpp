#include "test_visitor.h"

namespace StayNames {

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

void TestVisitor::ExpLeaf(Token type, const char *value)
{
    PrintIndent();
    fprintf(fd_, value);
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

void TestVisitor::NameOfType(const char *package, const char *name)
{
    PrintIndent();
    if (package == NULL) {
        fprintf(fd_, "TypeName %s", name);
    } else {
        fprintf(fd_, "TypeName %s.%s", package, name);
    }
}

void TestVisitor::BaseType(Token token)
{
    PrintIndent();
    fprintf(fd_, "%s type", lexer_->GetTokenString(token));
}

} // namespace