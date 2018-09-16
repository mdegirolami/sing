#include "AstBuilder.h"

namespace StayNames {

//////////////
//
// general
//
//////////////
void AstBuilder::BeginFile(const char *package_name)
{
    root = new AstFile;
    root->package_name = package_name;
    root->dependencies = NULL;
}

void AstBuilder::EndFile()
{
}

void AstBuilder::Dependency(const char *package_path, const char *local_name)
{
    AstDependency *scan, *desc;

    desc = new AstDependency;
    desc->next = NULL;
    desc->package_dir = package_path;
    desc->package_name = local_name;
    if (root->dependencies == NULL) {
        root->dependencies = desc;
    } else {
        for (scan = root->dependencies; scan->next != NULL; scan = scan->next);
        scan->next = desc;
    }
}

///////////////////
//
// declarations
//
///////////////////
void AstBuilder::BeginDecl(Token decl_type, bool is_member)
{
    switch (decl_type) {
    case TOKEN_VAR:
        {
            VarDeclaration *desc = new VarDeclaration;
            desc->volatile_flag = false;
            desc->initer = NULL;
            desc->type_spec = NULL;
            root->declarations.push_back(desc);
            stack.clear();
            stack.push_back(desc);
            type_receiver = &desc->type_spec;
        }
        break;
    case TOKEN_FUNC:
        {
            FuncDeclaration *desc = new FuncDeclaration;
            desc->is_class_member = is_member;
            desc->block = NULL;
            desc->type_spec = NULL;
            root->declarations.push_back(desc);
            stack.clear();
            stack.push_back(desc);
            type_receiver = &desc->type_spec;
        }
        break;
    default:
        Error("unsupported declaration");
        break;
    }
}

void AstBuilder::SetClassName(const char *classname)
{
    IAstNode *desc = stack[stack.size() - 1];
    FuncDeclaration *fundecl;

    if (desc->GetType() != ANT_FUNC) {
        Error("qualified name unallowed here");
    }
    fundecl = (FuncDeclaration*)desc;
    if (!fundecl->is_class_member) {
        Error("qualified name unallowed here");
    }
    fundecl->classname = classname;
}

void AstBuilder::SetName(const char *name)
{
    IAstNode *desc = stack[stack.size() - 1];
    switch (desc->GetType()) {
    case ANT_FUNC:
        ((FuncDeclaration*)desc)->name = name;
        break;
    case ANT_VAR:
        ((VarDeclaration*)desc)->name = name;
        break;
    default:
        Error("Ast node doesn't support naming");
    }
}

void AstBuilder::SetExternName(const char *package_name, const char *name)
{
}

void AstBuilder::SetVolatile(void)
{
    IAstNode *desc = stack[stack.size() - 1];

    if (desc->GetType() != ANT_VAR) {
        Error("only variables can be volatiles");
    }
    ((VarDeclaration*)desc)->volatile_flag = true;
}

void AstBuilder::BeginIniter(void)
{
}

void AstBuilder::EndIniter(void)
{
}


///////////////////
//
// types
//
///////////////////
void AstBuilder::SetBaseType(Token basetype)
{
    AstBaseType *desc = new AstBaseType;
    desc->base_type = basetype;
    *type_receiver = desc;
}

void AstBuilder::SetNamedType(const char *name)
{
    AstNamedType *desc = new AstNamedType;
    desc->name = name;
    *type_receiver = desc;
}

void AstBuilder::SetExternType(const char *package_name, const char *type_name)
{
    AstQualifiedType *desc = new AstQualifiedType;
    desc->package = package_name;
    desc->name = type_name;
    *type_receiver = desc;
}

void AstBuilder::SetArrayType(int num_elements)
{
    AstArrayOrMatrixType *desc = new AstArrayOrMatrixType;
    desc->is_matrix = false;
    desc->dimensions.reserve(4);
    desc->dimensions.push_back(num_elements);
    desc->next = NULL;
    *type_receiver = desc;
    type_receiver = &desc->next;
}

void AstBuilder::SetMatrixType(int num_elements)
{
}

void AstBuilder::AddADimension(int num_elements)
{
}

void AstBuilder::SetMapType(void)
{
}

void AstBuilder::SetPointerType(bool isconst, bool isweak)
{
}

void AstBuilder::SetFunctionType(bool ispure)
{
}

void AstBuilder::SetArgDeclaration(Token direction, const char *name)
{
}



///////////////////
//
// blocks
//
///////////////////
void AstBuilder::BeginBlock()
{
}

void AstBuilder::EndBlock()
{
}

void AstBuilder::AssignStatement(void)
{

}

void AstBuilder::UpdateStatement(void)
{

}

void AstBuilder::IncrementStatement(Token inc_or_dec)
{

}

void AstBuilder::EndOfStatement(void)
{

}

///////////////////
//
// expressions
//
///////////////////
void AstBuilder::BeginExpression()
{
}

void AstBuilder::SetExpLeaf(Token subtype, const char *value)
{
}

void AstBuilder::SetUnary(Token unop)
{
}

void AstBuilder::SetBinary(Token binop)
{
}

void AstBuilder::OpenBraces(void)
{
}

void AstBuilder::CloseBraces(void)
{
}

void AstBuilder::BeginFunctionArgs(void)
{
}

void AstBuilder::AddArgument(const char *name)
{
}

void AstBuilder::ReinterpretCast(void)
{
}

void AstBuilder::EndFunctionArgs(void)
{
}

void AstBuilder::BeginIndices(void)
{
}

void AstBuilder::StartIndex(void)
{
}

void AstBuilder::EndIndices(void)
{
}


} // namespace