#include <assert.h>
#include <string.h>
#include "ast_nodes.h"

namespace SingNames {

/////////////////////////
//
// TYPES
//
/////////////////////////
IAstTypeNode *SolveTypedefs(IAstTypeNode *begin)
{
    while (begin != nullptr) {
        if (begin->GetType() != ANT_NAMED_TYPE) {
            break;
        } else {
            // TODO: Multicomponent names !!
            TypeDeclaration *declaration = ((AstNamedType*)begin)->wp_decl_;
            if (declaration != nullptr) {
                begin = declaration->type_spec_;
            } else {
                begin = nullptr;
            }
        }
    }
    return(begin);
}

ParmPassingMethod GetParameterPassingMethod(IAstTypeNode *type_spec, bool input_parm)
{
    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
        if (input_parm && ((AstBaseType*)type_spec)->base_type_ == TOKEN_STRING) return(PPM_INPUT_STRING);
        // fallthrough
    case ANT_POINTER_TYPE:
    case ANT_FUNC_TYPE:
        return(input_parm ? PPM_VALUE : PPM_POINTER);
    case ANT_NAMED_TYPE:
        return(GetParameterPassingMethod(((AstNamedType*)type_spec)->wp_decl_->type_spec_, input_parm));
    case ANT_ARRAY_TYPE:
    case ANT_MAP_TYPE:
    default:
        break;
    }
    return(input_parm ? PPM_CONSTREF : PPM_POINTER);
}

AstFuncType::~AstFuncType()
{
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        if (arguments_[ii] != nullptr) delete arguments_[ii];
    }
    if (return_type_ != nullptr && is_owning_) delete return_type_;
}

bool AstFuncType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstFuncType *other = (AstFuncType*)src_tree;
    if (arguments_.size() != other->arguments_.size() || varargs_ != other->varargs_ || 
        ispure_ && !other->ispure_ || is_member_ != other->is_member_) {
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
    assert(pointed_type_ != nullptr);
    assert(other->pointed_type_ != nullptr);
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
    assert(key_type_ != nullptr);
    assert(other->key_type_ != nullptr);
    assert(returned_type_ != nullptr);
    assert(other->returned_type_ != nullptr);
    return(true);
}

AstArrayType::~AstArrayType()
{
    if (expression_ != nullptr) delete expression_;
    if (element_type_ != nullptr) delete element_type_;
}

bool AstArrayType::IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode)
{
    if (src_tree->GetType() != GetType()) return(false);
    AstArrayType *other = (AstArrayType*)src_tree;
    switch (mode) {
    case FOR_EQUALITY:
        if (is_dynamic_ != other->is_dynamic_ || dimension_ != other->dimension_) {
            return(false);
        }
        break;
    case FOR_ASSIGNMENT:
        if (!is_dynamic_ && (other->is_dynamic_ || dimension_ != other->dimension_)) {
            return(false);
        }
        break;
    case FOR_REFERENCING:
        if (is_dynamic_ != other->is_dynamic_ || dimension_ != other->dimension_) {
            return(false);
        }
        break;
    }
    assert(element_type_ != nullptr);
    assert(other->element_type_ != nullptr);
    return(true);
}

bool AstArrayType::SupportsEqualOperator(void) { 
    return(element_type_ != nullptr ? element_type_->SupportsEqualOperator() : false); 
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
    return(wp_decl_ != nullptr ? wp_decl_->type_spec_->SizeOf() : 0); 
}

bool AstNamedType::NeedsZeroIniter(void) { 
    return(wp_decl_ != nullptr ? wp_decl_->type_spec_->NeedsZeroIniter() : true); 
}

bool AstNamedType::SupportsEqualOperator(void) {
    return(wp_decl_ != nullptr ? wp_decl_->type_spec_->SupportsEqualOperator() : false); 
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
    case TOKEN_VOID:
        return(0);
    case TOKEN_BOOL:
        return(1);
    default:
        break;
    }
    return(0);
}

AstEnumType::~AstEnumType()
{
    for (int ii = 0; ii < (int)initers_.size(); ++ii) {
        IAstExpNode *initer = initers_[ii];
        if (initer != nullptr) delete initer;
    }
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

AstClassType::AstClassType()
{
    first_hinherited_member_ = -1;
    has_destructor = has_constructor = constructor_written = false;
    can_be_copied = true;
}

AstClassType::~AstClassType()
{
    for (int ii = 0; ii < (int)member_vars_.size(); ++ii) {
        VarDeclaration *member = member_vars_[ii];
        if (member != nullptr) delete member;
    }
    int top = (int)member_functions_.size();
    if (first_hinherited_member_ != -1) top = first_hinherited_member_;
    for (int ii = 0; ii < top; ++ii) {
        FuncDeclaration *member = member_functions_[ii];
        if (member != nullptr) {
            if (fn_implementors_[ii] != "") {
                member->function_type_ = nullptr;   // is not the owner !!
            }
            delete member;
        }
    }
    for (int ii = 0; ii < (int)member_interfaces_.size(); ++ii) {
        if (member_interfaces_[ii] != nullptr) delete member_interfaces_[ii];
    }
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

void AstClassType::AddMemberFun(FuncDeclaration *member, string implementor) {
    member_functions_.push_back(member);
    fn_implementors_.push_back(implementor);
    if (member->name_ == "finalize") {
        has_destructor = true;
        //can_be_copied = false;    // just a bad idea: imagine if we added a log in finalize() for debug purpose...
                                    // also, if you do this, remember to check arrays and maps who have non-copiable items.
                                    // the best would be to have a IAstTypeNode::SupportsEssignOperator() function.
    }
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
    wp_decl_ = nullptr;
    pkg_index_ = -1;
    real_is_int_ = real_is_negated_ = img_is_negated_ = false;
    unambiguous_member_access = false;
}

AstBinop::AstBinop(Token type, IAstExpNode *left, IAstExpNode*right) {
    subtype_ = type;
    operand_left_ = left;
    operand_right_ = right;
    builtin_ = nullptr;
    builtin_signature_ = nullptr;
}

AstBinop::~AstBinop() 
{ 
    if (operand_left_ != nullptr) delete operand_left_; 
    if (operand_right_ != nullptr) delete operand_right_; 
    if (builtin_ != nullptr) delete builtin_; 
}

AstFunCall::~AstFunCall()
{
    for (int ii = 0; ii < (int)arguments_.size(); ++ii) {
        if (arguments_[ii] != nullptr) delete arguments_[ii];
    }
    if (left_term_ != nullptr) delete left_term_;
}

AstArgument::~AstArgument()
{
    if (expression_ != nullptr) delete expression_;
}

AstIndexing::~AstIndexing()
{
    if (lower_value_ != nullptr) delete lower_value_;
    if (upper_value_ != nullptr) delete upper_value_;
    if (indexed_term_ != nullptr) delete indexed_term_;
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
        if (expressions_[ii] != nullptr) delete expressions_[ii];
    }
    for (ii = 0; ii < (int)blocks_.size(); ++ii) {
        if (blocks_[ii] != nullptr) delete blocks_[ii];
    }
    if (default_block_ != nullptr) delete default_block_;
}

AstFor::~AstFor() { 
    if (iterator_ != nullptr) delete iterator_;
    if (index_ != nullptr) delete index_;
    if (set_ != nullptr) delete set_;
    if (low_ != nullptr) delete low_;
    if (high_ != nullptr) delete high_;
    if (step_ != nullptr) delete step_;
    if (block_ != nullptr) delete block_;
}

AstBlock::~AstBlock()
{
    for (int ii = 0; ii < (int)block_items_.size(); ++ii) {
        if (block_items_[ii] != nullptr) delete block_items_[ii];
    }
}

AstSwitch::~AstSwitch()
{
    for (int ii = 0; ii < (int)case_values_.size(); ++ii) {
        if (case_values_[ii] != nullptr) delete case_values_[ii];
    }
    for (int ii = 0; ii < (int)statements_.size(); ++ii) {
        if (statements_[ii] != nullptr) delete statements_[ii];
    }
    if (switch_value_ != nullptr) delete switch_value_;
}

AstTypeSwitch::~AstTypeSwitch()
{
    for (int ii = 0; ii < (int)case_types_.size(); ++ii) {
        if (case_types_[ii] != nullptr) delete case_types_[ii];
    }
    for (int ii = 0; ii < (int)case_statements_.size(); ++ii) {
        if (case_statements_[ii] != nullptr) delete case_statements_[ii];
    }
    if (expression_ != nullptr) delete expression_;
    if (reference_ != nullptr) delete reference_;
}

/////////////////////////
//
// BASE STRUCTURE OF PACKAGE
//
/////////////////////////

AstIniter::~AstIniter()
{
    for (int ii = 0; ii < (int)elements_.size(); ++ii) {
        if (elements_[ii] != nullptr) delete elements_[ii];
    }
}

void VarDeclaration::SetUsageFlags(ExpressionUsage usage)
{
    if (HasOneOfFlags(VF_IS_REFERENCE)) {
        if (weak_iterated_var_ != nullptr) {
            weak_iterated_var_->SetUsageFlags(usage);
            return;
        }
    }
    switch (usage) {
    case ExpressionUsage::WRITE:
        SetFlags(VF_WASWRITTEN);
        break;
    case ExpressionUsage::READ:
        SetFlags(VF_WASREAD);
        break;
    case ExpressionUsage::NONE:
        break;
    case ExpressionUsage::BOTH:
        SetFlags(VF_WASREAD | VF_WASWRITTEN);
        break;
    }    
}

void FuncDeclaration::SetNames(const char *name1, const char *name2)
{
    if (name2 == nullptr || name2[0] == 0) {
        is_class_member_ = false;
        name_ = name1;
    } else {
        is_class_member_ = true;
        classname_ = name1;
        name_ = name2;
    }
}

AstDependency::AstDependency(const char *path, const char *name)
{
    package_dir_ = path;
    if (name == nullptr || name[0] == 0) {
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
        if (dependencies_[ii] != nullptr) delete dependencies_[ii];
    }
    for (int ii = 0; ii < (int)declarations_.size(); ++ii) {
        if (declarations_[ii] != nullptr) delete declarations_[ii];
    }
    for (int ii = 0; ii < (int)remarks_.size(); ++ii) {
        if (remarks_[ii] != nullptr) delete remarks_[ii];
    }
}

} // namespace