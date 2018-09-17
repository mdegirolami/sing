#include <assert.h>
#include "ast_nodes.h"

namespace StayNames {

/////////////////////////
//
// TYPES
//
/////////////////////////

void AstFuncPointerType::Visit(IAstVisitor *visitor)
{

}

bool AstFuncPointerType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstFuncPointerType *other = (AstFuncPointerType*)other_tree;
    if (ispure != other->ispure || varargs != other->varargs) return(false);
    if (arg == NULL) {
        if (other->arg != NULL) return(false);
    } else {
        if (other->arg == NULL || !arg->TreesAreEqual(other->arg)) return(false);
    }
    if (return_type == NULL) {
        if (other->return_type != NULL) return(false);
    } else {
        if (other->return_type == NULL || !return_type->TreesAreEqual(other->return_type)) return(false);
    }
    return(true);
}

void IAstArgument::Visit(IAstVisitor *visitor)
{

}

bool IAstArgument::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    IAstArgument *other = (IAstArgument*)other_tree;
    if (direction != other->direction) return(false);
    assert(type != NULL);
    assert(other->type != NULL);
    return(type->TreesAreEqual(other->type));
}

void AstPointerType::Visit(IAstVisitor *visitor)
{

}

bool AstPointerType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstPointerType *other = (AstPointerType*)other_tree;
    if (isconst != other->isconst) return(false);
    assert(next != NULL);
    assert(other->next != NULL);
    return(next->TreesAreEqual(other->next));
}

void AstMapType::Visit(IAstVisitor *visitor)
{

}

bool AstMapType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstMapType *other = (AstMapType*)other_tree;
    assert(key_type != NULL);
    assert(other->key_type != NULL);
    assert(next != NULL);
    assert(other->next != NULL);
    return(key_type->TreesAreEqual(key_type) && next->TreesAreEqual(other->next));
}

void AstArrayOrMatrixType::Visit(IAstVisitor *visitor)
{

}

bool AstArrayOrMatrixType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstArrayOrMatrixType *other = (AstArrayOrMatrixType*)other_tree;
    if (is_matrix != other->is_matrix || dimensions.size() != other->dimensions.size()) {
        return(false);
    }
    if (!is_matrix) {
        for (int ii = 0; ii < dimensions.size(); ++ii) {
            if (dimensions[ii] != other->dimensions[ii]) {
                return(false);
            }
        }
    }
    assert(next != NULL);
    assert(other->next != NULL);
    return(next->TreesAreEqual(other->next));
}

void AstQualifiedType::Visit(IAstVisitor *visitor)
{

}

bool AstQualifiedType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstQualifiedType *other = (AstQualifiedType*)other_tree;
    return(name == other->name && package == other->package);
}

void AstNamedType::Visit(IAstVisitor *visitor)
{

}

bool AstNamedType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstNamedType *other = (AstNamedType*)other_tree;
    return(name == other->name);
}

void AstBaseType::Visit(IAstVisitor *visitor)
{

}

bool AstBaseType::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    AstBaseType *other = (AstBaseType*)other_tree;
    return(base_type == other->base_type);
}


/////////////////////////
//
// EXPRESSIONS
//
/////////////////////////

void AstExpressionLeaf::Visit(IAstVisitor *visitor)
{

}

bool AstExpressionLeaf::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void AstUnop::Visit(IAstVisitor *visitor)
{

}

bool AstUnop::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void AstBinop::Visit(IAstVisitor *visitor)
{

}

bool AstBinop::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void AstFunCall::Visit(IAstVisitor *visitor)
{

}

bool AstFunCall::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

AstNamedArgument::~AstNamedArgument()
{
    if (expression != NULL) delete expression;
    if (cast_to != NULL) delete cast_to;
    if (next != NULL) delete next;
}

void AstNamedArgument::Visit(IAstVisitor *visitor)
{

}

bool AstNamedArgument::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

AstArgument::~AstArgument()
{
    if (expression != NULL) delete expression;
    if (cast_to != NULL) delete cast_to;
    if (next != NULL) delete next;
}

void AstArgument::Visit(IAstVisitor *visitor)
{

}

bool AstArgument::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

AstIndexing::~AstIndexing()
{
    for (int ii = 0; ii < expressions.size(); ++ii) {
        if (expressions[ii] != NULL) delete expressions[ii];
        if (upper_values[ii] != NULL) delete upper_values[ii];
    }
    if (left_term != NULL) delete left_term;
}

void AstIndexing::Visit(IAstVisitor *visitor)
{

}

bool AstIndexing::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}


/////////////////////////
//
// STATEMENTS
//
/////////////////////////

void AstIncDec::Visit(IAstVisitor *visitor)
{

}

bool AstIncDec::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void AstUpdate::Visit(IAstVisitor *visitor)
{

}

bool AstUpdate::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

AstAssignment::~AstAssignment()
{
    for (int ii = 0; ii < left_terms.size(); ++ii) {
        if (left_terms[ii] != NULL) delete left_terms[ii];
        if (right_terms[ii] != NULL) delete right_terms[ii];
    }
}

void AstAssignment::Visit(IAstVisitor *visitor)
{

}

bool AstAssignment::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void AstBlock::Visit(IAstVisitor *visitor)
{

}

bool AstBlock::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}


/////////////////////////
//
// BASE STRUCTURE OF PACKAGE
//
/////////////////////////


void AstIniter::Visit(IAstVisitor *visitor)
{

}

bool AstIniter::TreesAreEqual(IAstNode *other_tree)
{
    if (other_tree->GetType() != GetType()) return(false);
    return(false);
}

void VarDeclaration::Visit(IAstVisitor *visitor)
{

}

bool VarDeclaration::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void FuncDeclaration::Visit(IAstVisitor *visitor)
{

}

bool FuncDeclaration::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

void AstDependency::Visit(IAstVisitor *visitor)
{

}

bool AstDependency::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

AstFile::~AstFile()
{
    if (dependencies != NULL) delete dependencies;
    for (int ii = 0; ii < declarations.size(); ++ii) {
        if (declarations[ii] != NULL) delete declarations[ii];
    }
}

void AstFile::Visit(IAstVisitor *visitor)
{

}

bool AstFile::TreesAreEqual(IAstNode *other_tree)
{
    return(false);
}

} // namespace