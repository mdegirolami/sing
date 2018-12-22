#include <assert.h>
#include <string.h>
#include "ast_nodes.h"

namespace SingNames {

/////////////////////////
//
// TYPES
//
/////////////////////////

AstFuncType::~AstFuncType()
{
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        if (arguments_[ii] != NULL) delete arguments_[ii];
    }
    if (return_type_ != NULL) delete return_type_;
}

void AstFuncType::Visit(IAstVisitor *visitor)
{
    visitor->BeginFuncType(ispure_, varargs_, arguments_.size());
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        arguments_[ii]->Visit(visitor);
    }
    if (return_type_ != NULL) return_type_->Visit(visitor);
    visitor->EndFuncType(ispure_, varargs_, arguments_.size());
}

bool AstFuncType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstFuncType *other = (AstFuncType*)src_tree;
    if (arguments_.size() != other->arguments_.size() || varargs_ != other->varargs_ || ispure_ && !other->ispure_) {
        return(false);
    }
    if (mode == FOR_EQUALITY && ispure_ != other->ispure_) {
        return(false);
    }
    return(true);
}

void AstArgumentDecl::Visit(IAstVisitor *visitor)
{
    visitor->BeginArgumentDecl(direction_, name_.c_str(), initer_ != NULL);
    type_->Visit(visitor);
    if (initer_ != NULL) initer_->Visit(visitor);
    visitor->EndArgumentDecl(direction_, name_.c_str(), initer_ != NULL);
}

bool AstArgumentDecl::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstArgumentDecl *other = (AstArgumentDecl*)src_tree;
    if (direction_ != other->direction_) return(false);
    assert(type_ != NULL);
    assert(other->type_ != NULL);
    return(true);
}

void AstPointerType::Visit(IAstVisitor *visitor)
{
    visitor->BeginPointerType(isconst_, isweak_);
    pointed_type_->Visit(visitor);
    visitor->EndPointerType(isconst_, isweak_);
}

bool AstPointerType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstPointerType *other = (AstPointerType*)src_tree;
    if (!isconst_ && other->isconst_) return(false);
    if (mode == FOR_EQUALITY && (isconst_ != other->isconst_ || isweak_ != other->isweak_)) {
        return(false);
    }
    if (mode == WITH_OUTPUT_DECLARATION && isweak_ != other->isweak_) {
        return(false);
    }
    assert(pointed_type_ != NULL);
    assert(other->pointed_type_ != NULL);
    return(true);
}

void AstMapType::Visit(IAstVisitor *visitor)
{
    visitor->BeginMapType();
    key_type_->Visit(visitor);
    visitor->MapReturnType();
    returned_type_->Visit(visitor);
    visitor->EndMapType();
}

bool AstMapType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstMapType *other = (AstMapType*)src_tree;
    assert(key_type_ != NULL);
    assert(other->key_type_ != NULL);
    assert(returned_type_ != NULL);
    assert(other->returned_type_ != NULL);
    return(true);
}

AstArrayOrMatrixType::~AstArrayOrMatrixType()
{
    for (int ii = 0; ii < (int)expressions_.size(); ++ii) {
        if (expressions_[ii] != NULL) delete expressions_[ii];
    }
    if (element_type_ != NULL) delete element_type_;
}

void AstArrayOrMatrixType::Visit(IAstVisitor *visitor)
{
    visitor->BeginArrayOrMatrixType(is_matrix_, expressions_.size());
    for (int ii = 0; ii < (int)expressions_.size(); ++ii) {
        visitor->ConstIntExpressionValue(dimensions_[ii]);
        if (expressions_[ii] != NULL) expressions_[ii]->Visit(visitor);
    }
    element_type_->Visit(visitor);
    visitor->EndArrayOrMatrixType(is_matrix_, expressions_.size());
}

bool AstArrayOrMatrixType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstArrayOrMatrixType *other = (AstArrayOrMatrixType*)src_tree;
    if (expressions_.size() != other->expressions_.size()) {
        return(false);
    }
    for (int ii = 0; ii < (int)dimensions_.size(); ++ii) {
        if (dimensions_[ii] != other->dimensions_[ii]) {

            // for assignment, we fail only if both dimensions are known at compile time 
            if (mode == FOR_EQUALITY || dimensions_[ii] * other->dimensions_[ii] > 0) {
                return(false);
            }
        }
    }
    assert(element_type_ != NULL);
    assert(other->element_type_ != NULL);
    return(true);
}

void AstQualifiedType::Visit(IAstVisitor *visitor)
{
    for (int ii = 0; ii < names_.GetNamesCount(); ++ii) {
        visitor->NameOfType(names_.GetName(ii), ii);
    }
}

bool AstQualifiedType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstQualifiedType *other = (AstQualifiedType*)src_tree;
    return(names_.IsEqualTo(&other->names_));
}

void AstQualifiedType::BuildTheFullName(string *dst)
{
    const char *component;
    int     ii = 0;

    do {
        component = names_.GetName(ii);
        if (component != NULL) {
            if (ii != 0) {
                *dst += "::";
            }
            *dst += component;
        }
    } while (component != NULL);
}

void AstNamedType::Visit(IAstVisitor *visitor)
{
    visitor->NameOfType(name_.c_str(), 0);
}

bool AstNamedType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstNamedType *other = (AstNamedType*)src_tree;
    return(name_ == other->name_);
}

void AstBaseType::Visit(IAstVisitor *visitor)
{
    visitor->BaseType(base_type_);
}

bool AstBaseType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstBaseType *other = (AstBaseType*)src_tree;
    return(base_type_ == other->base_type_);
}

/////////////////////////
//
// EXPRESSIONS
//
/////////////////////////

void AstExpressionLeaf::Visit(IAstVisitor *visitor)
{
    visitor->ExpLeaf(subtype_, value_.c_str());
}

void AstUnop::Visit(IAstVisitor *visitor)
{
    visitor->BeginUnop(subtype_);
    operand_->Visit(visitor);
    visitor->EndUnop(subtype_);
}

void AstBinop::Visit(IAstVisitor *visitor)
{
    visitor->BeginBinop(subtype_);
    operand_left_->Visit(visitor);
    visitor->BeginBinopSecondArg();
    operand_right_->Visit(visitor);
    visitor->EndBinop(subtype_);
}

AstFunCall::~AstFunCall()
{
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        if (arguments_[ii] != NULL) delete arguments_[ii];
    }
    if (left_term_ != NULL) delete left_term_;
}

void AstFunCall::Visit(IAstVisitor *visitor)
{
    visitor->BeginFunCall();
    left_term_->Visit(visitor);
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        visitor->FunCallArg(ii);
        if (arguments_[ii] != NULL) {
            arguments_[ii]->Visit(visitor);
        }
    }
    visitor->EndFunCall();
}

AstArgument::~AstArgument()
{
    if (expression_ != NULL) delete expression_;
    if (cast_to_ != NULL) delete cast_to_;
}

void AstArgument::Visit(IAstVisitor *visitor)
{
    visitor->BeginArgument(name_.c_str());
    expression_->Visit(visitor);
    if (cast_to_ != NULL) {
        visitor->CastTypeBegin();
        cast_to_->Visit(visitor);
        visitor->CastTypeEnd();
    }
    visitor->EndArgument(name_.c_str());
}

AstIndexing::~AstIndexing()
{
    for (int ii = 0; ii < (int)lower_values_.size(); ++ii) {
        if (lower_values_[ii] != NULL) delete lower_values_[ii];
    }
    for (int ii = 0; ii < (int)upper_values_.size(); ++ii) {
        if (upper_values_[ii] != NULL) delete upper_values_[ii];
    }
    if (left_term_ != NULL) delete left_term_;
}

void AstIndexing::Visit(IAstVisitor *visitor)
{
    visitor->BeginIndexing();
    left_term_->Visit(visitor);
    for (int ii = 0; ii < (int)lower_values_.size(); ++ii) {
        visitor->Index(ii, lower_values_[ii] != NULL, upper_values_[ii] != NULL);
        if (lower_values_[ii] != NULL) {
            lower_values_[ii]->Visit(visitor);
        }
        if (upper_values_[ii] != NULL) {
            upper_values_[ii]->Visit(visitor);
        }
    }
    visitor->EndIndexing();
}

/////////////////////////
//
// STATEMENTS
//
/////////////////////////

void AstIncDec::Visit(IAstVisitor *visitor)
{
    visitor->BeginIncDec(operation_);
    left_term_->Visit(visitor);
    visitor->EndIncDec(operation_);
}

void AstUpdate::Visit(IAstVisitor *visitor)
{
    visitor->BeginUpdateStatement(operation_);
    visitor->BeginLeftTerm(0);
    left_term_->Visit(visitor);
    visitor->BeginRightTerm(0);
    right_term_->Visit(visitor);
    visitor->EndUpdateStatement(operation_);
}

AstAssignment::~AstAssignment()
{
    for (int ii = 0; ii < (int)left_terms_.size(); ++ii) {
        if (left_terms_[ii] != NULL) delete left_terms_[ii];
        if (right_terms_[ii] != NULL) delete right_terms_[ii];
    }
}

void AstAssignment::Visit(IAstVisitor *visitor)
{
    int ii;

    visitor->BeginAssignments(left_terms_.size());
    for (ii = 0; ii < (int)left_terms_.size(); ++ii) {
        visitor->BeginLeftTerm(ii);
        left_terms_[ii]->Visit(visitor);
    }
    for (ii = 0; ii < (int)left_terms_.size(); ++ii) {
        visitor->BeginRightTerm(ii);
        right_terms_[ii]->Visit(visitor);
    }
    visitor->EndAssignments(left_terms_.size());
}

void AstWhile::Visit(IAstVisitor *visitor)
{
    visitor->BeginWhile();
    expression_->Visit(visitor);
    block_->Visit(visitor);
    visitor->EndWhile();
}

AstIf::~AstIf()
{
    int ii;

    for (ii = 0; ii < (int)expressions_.size(); ++ii) {
        if (expressions_[ii] != NULL) delete expressions_[ii];
    }
    for (ii = 0; ii < (int)blocks_.size(); ++ii) {
        if (blocks_[ii] != NULL) delete blocks_[ii];
    }
    if (default_block_ != NULL) delete default_block_;
}

void AstIf::Visit(IAstVisitor *visitor)
{
    int ii;
    
    visitor->BeginIf();
    for (ii = 0; ii < (int)expressions_.size(); ++ii) {
        visitor->BeginIfClause(ii);
        expressions_[ii]->Visit(visitor);
        blocks_[ii]->Visit(visitor);
        visitor->EndIfClause(ii);
    }
    if (default_block_ != NULL) {
        default_block_->Visit(visitor);
    }
    visitor->EndIf();
}

AstFor::~AstFor() { 
    if (set_ != NULL) delete set_;
    if (low_ != NULL) delete low_;
    if (high_ != NULL) delete high_;
    if (step_ != NULL) delete step_;
    if (block_ != NULL) delete block_;
}

void AstFor::Visit(IAstVisitor *visitor)
{
    visitor->BeginFor(index_name_.c_str(), iterator_name_.c_str());
    if (set_ != NULL) {
        visitor->BeginForSet();
        set_->Visit(visitor);
        visitor->EndForSet();
    }
    if (low_ != NULL) {
        visitor->BeginForLow();
        low_->Visit(visitor);
        visitor->EndForLow();
    }
    if (high_ != NULL) {
        visitor->BeginForHigh();
        high_->Visit(visitor);
        visitor->EndForHigh();
    }
    if (step_ != NULL) {
        visitor->BeginForStep();
        step_->Visit(visitor);
        visitor->EndForStep();
    }
    block_->Visit(visitor);
    visitor->EndFor(index_name_.c_str(), iterator_name_.c_str());
}

void AstSimpleStatement::Visit(IAstVisitor *visitor)
{
    visitor->SimpleStatement(subtype_);
}

void AstReturn::Visit(IAstVisitor *visitor)
{
    visitor->BeginReturn();
    if (retvalue_ != NULL) {
        retvalue_->Visit(visitor);
    }
    visitor->EndReturn();
}

AstBlock::~AstBlock()
{
    for (int ii = 0; ii < (int)block_items_.size(); ++ii) {
        if (block_items_[ii] != NULL) delete block_items_[ii];
    }
}

void AstBlock::Visit(IAstVisitor *visitor)
{
    visitor->BeginBlock();
    for (int ii = 0; ii < (int)block_items_.size(); ++ii) {
        block_items_[ii]->Visit(visitor);
    }
    visitor->EndBlock();
}

/////////////////////////
//
// BASE STRUCTURE OF PACKAGE
//
/////////////////////////

AstIniter::~AstIniter()
{
    for (int ii = 0; ii < (int)elements_.size(); ++ii) {
        if (elements_[ii] != NULL) delete elements_[ii];
    }
}

void AstIniter::Visit(IAstVisitor *visitor)
{
    visitor->BeginIniter();
    for (int ii = 0; ii < (int)elements_.size(); ++ii) {
        elements_[ii]->Visit(visitor);
    }
    visitor->EndIniter();
}

void VarDeclaration::Visit(IAstVisitor *visitor)
{
    visitor->BeginVarDeclaration(name_.c_str(), volatile_flag_, initer_ != NULL);
    if (type_spec_ != NULL) {
        type_spec_->Visit(visitor);
    }
    if (initer_ != NULL) {
        initer_->Visit(visitor);
    }
    visitor->EndVarDeclaration(name_.c_str(), volatile_flag_, type_spec_ != NULL);
}


void ConstDeclaration::Visit(IAstVisitor *visitor)
{
    visitor->BeginConstDeclaration(name_.c_str());
    if (type_spec_ != NULL) {
        type_spec_->Visit(visitor);
    }
    if (initer_ != NULL) {
        initer_->Visit(visitor);
    }
    visitor->EndConstDeclaration(name_.c_str());
}

void TypeDeclaration::Visit(IAstVisitor *visitor)
{
    visitor->BeginTypeDeclaration(name_.c_str());
    type_spec_->Visit(visitor);
    visitor->EndTypeDeclaration(name_.c_str());
}

FuncDeclaration::FuncDeclaration(const char *name1, const char *name2)
{
    if (name2 == NULL || name2[0] == 0) {
        is_class_member_ = false;
        name_ = name1;
    } else {
        is_class_member_ = true;
        classname_ = name1;
        name_ = name2;
    }
    function_type_ = NULL;
    block_ = NULL;
}

void FuncDeclaration::Visit(IAstVisitor *visitor)
{
    visitor->BeginFuncDeclaration(name_.c_str(), is_class_member_, classname_.c_str());
    function_type_->Visit(visitor);
    block_->Visit(visitor);
    visitor->EndFuncDeclaration(name_.c_str(), is_class_member_, classname_.c_str());
}

AstDependency::AstDependency(const char *path, const char *name)
{
    package_dir_ = path;
    if (name == NULL || name[0] == 0) {
        const char *slash;

        for (slash = path + strlen(path) - 1; slash > path && *slash != '\\' && *slash != '/'; --slash);
        if (slash != path) ++slash;
        package_name_ = slash;
    } else {
        package_name_ = name;
    }
}

void AstDependency::Visit(IAstVisitor *visitor)
{
    visitor->PackageRef(package_dir_.c_str(), package_name_.c_str());
}

AstFile::~AstFile()
{
    for (int ii = 0; ii < (int)dependencies_.size(); ++ii) {
        if (dependencies_[ii] != NULL) delete dependencies_[ii];
    }
    for (int ii = 0; ii < (int)declarations_.size(); ++ii) {
        if (declarations_[ii] != NULL) delete declarations_[ii];
    }
}

void AstFile::Visit(IAstVisitor *visitor)
{
    visitor->File(package_name_.c_str());
    for (int ii = 0; ii < (int)dependencies_.size(); ++ii) {
        dependencies_[ii]->Visit(visitor);
    }
    for (int ii = 0; ii < (int)declarations_.size(); ++ii) {
        declarations_[ii]->Visit(visitor);
    }
}

ParmDirection Token2ParmDirection(Token token)
{
    switch (token) {
    case TOKEN_IN: return(PD_IN);
    case TOKEN_OUT: return(PD_OUT);
    case TOKEN_IO: return(PD_IO);
    default: break;
    }
    return(PD_ABSENT);
}

} // namespace