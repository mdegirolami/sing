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

bool AstFuncType::ReturnsVoid(void)
{
    return(return_type_ != nullptr && return_type_->GetType() == ANT_BASE_TYPE && ((AstBaseType*)return_type_)->base_type_ == TOKEN_VOID);
}

// ignore constness which must be tested with CheckConstness()
// this was done to give specific error messages
bool AstPointerType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstPointerType *other = (AstPointerType*)src_tree;
    if (mode == FOR_EQUALITY && isweak_ != other->isweak_) {
        return(false);
    }
    if (mode == FOR_REFERENCING && isweak_ != other->isweak_) {
        return(false);
    }
    assert(pointed_type_ != NULL);
    assert(other->pointed_type_ != NULL);
    return(true);
}

bool AstPointerType::CheckConstness(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstPointerType *other = (AstPointerType*)src_tree;
    if (!isconst_ && other->isconst_) return(false);
    if (mode == FOR_EQUALITY && isconst_ != other->isconst_) {
        return(false);
    }
    return(true);
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

AstArrayType::~AstArrayType()
{
    if (expression_ != NULL) delete expression_;
    if (element_type_ != NULL) delete element_type_;
}

bool AstArrayType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstArrayType *other = (AstArrayType*)src_tree;
    if (dimension_ != other->dimension_) {

        // for assignment, we fail only if both dimensions are known at compile time 
        if (mode == FOR_EQUALITY || dimension_ * other->dimension_ > 0) {
            return(false);
        }
    }
    assert(element_type_ != NULL);
    assert(other->element_type_ != NULL);
    return(true);
}

bool AstNamedType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstNamedType *other = (AstNamedType*)src_tree;
    if (name_ != other->name_) return(false);
    if (next_component == nullptr || other->next_component == nullptr) {
        if (next_component != nullptr || other->next_component != nullptr) {
            return(false);
        }
        return(true);
    }
    return(next_component->IsCompatible(other->next_component, mode));
}

int AstNamedType::SizeOf(void)
{ 
    return(wp_decl_ != NULL ? wp_decl_->type_spec_->SizeOf() : 0); 
}

void AstNamedType::AppendFullName(string *fullname)
{
    *fullname += name_;
    AstNamedType *scan = next_component;
    while (scan != nullptr) {
        *fullname += '.';
        *fullname += scan->name_;
        scan = scan->next_component;
    }
}

bool AstBaseType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstBaseType *other = (AstBaseType*)src_tree;
    return(base_type_ == other->base_type_);
}

int AstBaseType::SizeOf(void)
{
    switch (base_type_) {
    case TOKEN_INT8:
    case TOKEN_UINT8:
        return(1);
    case TOKEN_INT16:
    case TOKEN_UINT16:
        return(2);
    case TOKEN_INT32:
    case TOKEN_UINT32:
    case TOKEN_FLOAT32:
        return(4);
    case TOKEN_INT64:
    case TOKEN_UINT64:
    case TOKEN_FLOAT64:
    case TOKEN_COMPLEX64:
        return(8);
    case TOKEN_COMPLEX128:
        return(16);
    case TOKEN_STRING:
    case TOKEN_ERRORCODE:
    case TOKEN_VOID:
        return(0);
    case TOKEN_BOOL:
        return(1);
    default:
        break;
    }
    return(0);
}

bool AstEnumType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    // there is a single declaration for each enum/class/if type
    if (src_tree->GetType() != GetType()) return(false);
    if ((AstEnumType*)src_tree != this) return(false);
    return(true);
}

int AstEnumType::SizeOf(void)
{
    return(0);
}

bool AstInterfaceType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    // there is a single declaration for each enum/class/if type
    if (src_tree->GetType() != GetType()) return(false);
    if ((AstInterfaceType*)src_tree != this) return(false);
    return(true);
}

int AstInterfaceType::SizeOf(void)
{
    return(0);
}

bool AstInterfaceType::HasInterface(AstInterfaceType *intf)
{
    if (intf == this) return(true);
    for (int ii = 0; ii < (int)ancestors_.size(); ++ii) {
        AstNamedType *nt = ancestors_[ii];
        while (nt != nullptr && nt->wp_decl_ != nullptr && nt->wp_decl_->type_spec_ != nullptr) {
            IAstTypeNode *node = nt->wp_decl_->type_spec_;
            if (node->GetType() == ANT_INTERFACE_TYPE) {
                AstInterfaceType *found_if = (AstInterfaceType*)node;
                if (found_if == intf || found_if->HasInterface(intf)) {
                    return(true);
                }
                nt = nullptr;
            } else if (node->GetType() == ANT_NAMED_TYPE) {
                nt = (AstNamedType*)node;
            } else {
                nt = nullptr;
            }
        }
    }
    return(false);
}

bool AstClassType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    // there is a single declaration for each enum/class/if type
    if (src_tree->GetType() != GetType()) return(false);
    if ((AstClassType*)src_tree != this) return(false);
    return(true);
}

int AstClassType::SizeOf(void)
{
    return(0);
}

bool AstClassType::HasInterface(AstInterfaceType *intf)
{
    for (int ii = 0; ii < (int)member_interfaces_.size(); ++ii) {
        AstNamedType *nt = member_interfaces_[ii];
        while (nt != nullptr && nt->wp_decl_ != nullptr && nt->wp_decl_->type_spec_ != nullptr) {
            IAstTypeNode *node = nt->wp_decl_->type_spec_;
            if (node->GetType() == ANT_INTERFACE_TYPE) {
                AstInterfaceType *found_if = (AstInterfaceType*)node;
                if (found_if == intf || found_if->HasInterface(intf)) {
                    return(true);
                }
                nt = nullptr;
            } else if (node->GetType() == ANT_NAMED_TYPE) {
                nt = (AstNamedType*)node;
            } else {
                nt = nullptr;
            }
        }
    }
    return(false);
}

/////////////////////////
//
// EXPRESSIONS
//
/////////////////////////
AstExpressionLeaf::AstExpressionLeaf(Token type, const char *value)
{
    subtype_ = type;
    value_ = value;
    wp_decl_ = NULL;
    pkg_index_ = -1;
    real_is_int_ = real_is_negated_ = img_is_negated_ = false;
}

AstFunCall::~AstFunCall()
{
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        if (arguments_[ii] != NULL) delete arguments_[ii];
    }
    if (left_term_ != NULL) delete left_term_;
}

AstArgument::~AstArgument()
{
    if (expression_ != NULL) delete expression_;
    if (cast_to_ != NULL) delete cast_to_;
}

AstIndexing::~AstIndexing()
{
    if (lower_value_ != NULL) delete lower_value_;
    if (upper_value_ != NULL) delete upper_value_;
    if (indexed_term_ != NULL) delete indexed_term_;
}

/////////////////////////
//
// STATEMENTS
//
/////////////////////////
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

AstFor::~AstFor() { 
    if (iterator_ != NULL) delete iterator_;
    if (index_ != NULL) delete index_;
    if (set_ != NULL) delete set_;
    if (low_ != NULL) delete low_;
    if (high_ != NULL) delete high_;
    if (step_ != NULL) delete step_;
    if (block_ != NULL) delete block_;
}

AstBlock::~AstBlock()
{
    for (int ii = 0; ii < (int)block_items_.size(); ++ii) {
        if (block_items_[ii] != NULL) delete block_items_[ii];
    }
}

AstSwitch::~AstSwitch()
{
    for (int ii = 0; ii < (int)case_values_.size(); ++ii) {
        if (case_values_[ii] != NULL) delete case_values_[ii];
    }
    for (int ii = 0; ii < (int)case_statements_.size(); ++ii) {
        if (case_statements_[ii] != NULL) delete case_statements_[ii];
    }
    if (switch_value_ != NULL) delete switch_value_;
}

AstTypeSwitch::~AstTypeSwitch()
{
    for (int ii = 0; ii < (int)case_types_.size(); ++ii) {
        if (case_types_[ii] != NULL) delete case_types_[ii];
    }
    for (int ii = 0; ii < (int)case_statements_.size(); ++ii) {
        if (case_statements_[ii] != NULL) delete case_statements_[ii];
    }
    if (expression_ != NULL) delete expression_;
    if (reference_ != NULL) delete reference_;
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

void FuncDeclaration::SetNames(const char *name1, const char *name2)
{
    if (name2 == NULL || name2[0] == 0) {
        is_class_member_ = false;
        name_ = name1;
    } else {
        is_class_member_ = true;
        classname_ = name1;
        name_ = name2;
    }
}

AstEnumType::~AstEnumType()
{
    for (int ii = 0; ii < (int)initers_.size(); ++ii) {
        IAstExpNode *initer = initers_[ii];
        if (initer != nullptr) delete initer;
    }
}

AstInterfaceType::~AstInterfaceType()
{
    for (int ii = 0; ii < (int)ancestors_.size(); ++ii) {
        if (ancestors_[ii] != nullptr) delete ancestors_[ii];
    }
    for (int ii = 0; ii < (int)members_.size(); ++ii) {
        FuncDeclaration *member = members_[ii];
        if (member != nullptr) delete member;
    }
}

AstClassType::~AstClassType()
{
    for (int ii = 0; ii < (int)member_vars_.size(); ++ii) {
        VarDeclaration *member = member_vars_[ii];
        if (member != nullptr) delete member;
    }
    for (int ii = 0; ii < (int)member_functions_.size(); ++ii) {
        FuncDeclaration *member = member_functions_[ii];
        if (member != nullptr) delete member;
    }
    for (int ii = 0; ii < (int)member_interfaces_.size(); ++ii) {
        if (member_interfaces_[ii] != nullptr) delete member_interfaces_[ii];
    }
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
    package_index_ = -1;
    ambiguous_ = false;
    usage_ = DependencyUsage::UNUSED;
}

void AstDependency::SetUsage(DependencyUsage usage)
{
    if (usage_ == DependencyUsage::UNUSED || usage_ == DependencyUsage::PRIVATE && usage == DependencyUsage::PUBLIC) {
        usage_ = usage;
    }
}

AstFile::~AstFile()
{
    for (int ii = 0; ii < (int)dependencies_.size(); ++ii) {
        if (dependencies_[ii] != NULL) delete dependencies_[ii];
    }
    for (int ii = 0; ii < (int)declarations_.size(); ++ii) {
        if (declarations_[ii] != NULL) delete declarations_[ii];
    }
    for (int ii = 0; ii < (int)remarks_.size(); ++ii) {
        if (remarks_[ii] != NULL) delete remarks_[ii];
    }
}

} // namespace