#include <assert.h>
#include <float.h>
#include <math.h>
#include "ast_checks.h"

namespace SingNames {

bool AstChecker::CheckAll(AstFile *root)
{
    errors_.Reset();
    root_ = root;
    loops_nesting = 0;
    for (current_ = 0; current_ < root->declarations_.size(); ++current_) {        
        IAstNode *declaration = root->declarations_[current_];
        switch (declaration->GetType()) {
        case ANT_VAR:
            CheckVar((VarDeclaration*)declaration);
            break;
        case ANT_CONST:
            CheckConst((ConstDeclaration*)declaration);
            break;
        case ANT_TYPE:
            CheckType((TypeDeclaration*)declaration);
            break;
        case ANT_FUNC:
            CheckFunc((FuncDeclaration*)declaration);
            break;
        default:
            assert(false);
        }
    }
    return(errors_.GetNamesCount() == 0);
}

void AstChecker::CheckVar(VarDeclaration *declaration)
{
    if (declaration->type_spec_ != NULL) {
        if (CheckTypeSpecification(declaration->type_spec_, TSCM_STD)) {
            CheckIniter(declaration->type_spec_, declaration->initer_);
        }
    } else if (declaration->initer_ == NULL || declaration->initer_->GetType() == ANT_INITER) {
        Error("Auto variables require a single expression initer");
    } else {
        ExpressionAttributes attr;
        CheckExpression((IAstExpNode*)declaration->initer_, &attr);
        if (!attr.IsOnError() && !attr.IsGoodForAuto()) {
            Error("Initer for an auto variable must be typed (literals are not typed !)");
        }
    }
    InsertName(declaration->name_.c_str(), declaration);
}

void AstChecker::CheckConst(ConstDeclaration *declaration)
{
    if (CheckTypeSpecification(declaration->type_spec_, TSCM_STD)) {
        CheckIniter(declaration->type_spec_, declaration->initer_);
    }
    InsertName(declaration->name_.c_str(), declaration);
}

void AstChecker::CheckType(TypeDeclaration *declaration)
{
    CheckTypeSpecification(declaration->type_spec_, TSCM_STD);
    InsertName(declaration->name_.c_str(), declaration);
}

void AstChecker::CheckFunc(FuncDeclaration *declaration)
{
    CheckTypeSpecification(declaration->function_type_, TSCM_FUNC_BODY);
    CheckBlock(declaration->block_);
    InsertName(declaration->name_.c_str(), declaration);
}

bool AstChecker::CheckTypeSpecification(IAstNode *type_spec, TypeSpecCheckMode mode)
{
    bool success = true;
    char errmess[100];

    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
        if (mode != TSCM_RETVALUE && ((AstBaseType*)type_spec)->base_type_ == TOKEN_VOID) {
            Error("Void only allowed as a function return type");
            success = false;
        }
        break;
    case ANT_NAMED_TYPE:
        {
            const char *name = ((AstNamedType*)type_spec)->name_.c_str();
            IAstDeclarationNode *decl = FindDeclaration(name);
            if (decl == NULL) {
                sprintf(errmess, "Can't find type %s", name);
                Error(errmess);
                success = false;
            } else if (decl->GetType() != ANT_TYPE) {
                sprintf(errmess, "%s Should be a typedef", name);
                Error(errmess);
                success = false;
            } else {
                ((AstNamedType*)type_spec)->wp_decl_ = (TypeDeclaration*)decl;
            }
        }
        break;
    case ANT_QUALIFIED_TYPE:
        // TO BE IMPLEMENTED
        break;
    case ANT_ARRAY_TYPE:
        {
            AstArrayOrMatrixType *node = (AstArrayOrMatrixType*)type_spec;
            if (!CheckArrayIndices(node)) {
                success = false;
            }
            if (!CheckTypeSpecification(node->element_type_, TSCM_STD)) {
                success = false;
            }
        }
        break;
    case ANT_MAP_TYPE:
        {
            AstMapType *node = (AstMapType*)type_spec;
            if (!CheckTypeSpecification(node->key_type_, TSCM_STD)) {
                success = false;
            }
            if (!CheckTypeSpecification(node->returned_type_, TSCM_STD)) {
                success = false;
            }
        }
        break;
    case ANT_POINTER_TYPE:
        {
            AstPointerType *node = (AstPointerType*)type_spec;
            if (!CheckTypeSpecification(node->pointed_type_, TSCM_STD)) {
                success = false;
            }
        }
        break;
    case ANT_FUNC_TYPE:
        {
            AstFuncType *node = (AstFuncType*)type_spec;
            int ii;

            for (ii = 0; ii < node->arguments_.size(); ++ii) {
                AstArgumentDecl *arg = node->arguments_[ii];
                if (!CheckTypeSpecification(arg->type_, TSCM_STD)) {
                    success = false;
                } else {
                    if (arg->initer_ != NULL) {
                        if (arg->direction_ == PD_ABSENT) {
                            arg->direction_ = PD_IN;
                        } else if (arg->direction_ != PD_IN) {
                            Error("You can apply argument initers only to input parameters");
                            success = false;
                        } else if (!CheckIniter(arg->type_, arg->initer_)) {
                            success = false;
                        }
                    }
                }
                if (mode != TSCM_FUNC_BODY) {
                    if (arg->direction_ == PD_ABSENT) {
                        Error("Please specify the argument direction (in, out, io)");
                        success = false;
                    }
                }
            }
            if (!CheckTypeSpecification(node->return_type_, TSCM_RETVALUE)) {
                success = false;
            }
        }
        break;
    default:
        break;
    }
    return(success);
}

bool AstChecker::CheckIniter(IAstTypeNode *type_spec, IAstNode *initer)
{
    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
    case ANT_POINTER_TYPE:
    case ANT_FUNC_TYPE:
    case ANT_NAMED_TYPE:
        if (initer->GetType() == ANT_INITER) {
            Error("Only a single value is allowed here (not an aggregate initializer)");
            return(false);
        } else {
            ExpressionAttributes dst_attr, src_attr;

            dst_attr.InitWithTree(type_spec, true, this);
            CheckExpression((IAstExpNode*)initer, &src_attr);
            if (dst_attr.IsOnError() || src_attr.IsOnError()) {
                return(false);
            }
            return(CanAssign(&dst_attr, &src_attr));
        }
        break;
    case ANT_QUALIFIED_TYPE:
        // TODO
        return(false);
    case ANT_ARRAY_TYPE:
        if (initer->GetType() != ANT_INITER) {
            Error("You must init an aggregate with an aggregate initializer !");
            return(false);
        } else if (!CheckArrayIniter((AstArrayOrMatrixType*)type_spec, 0, initer)) {
            return(false);
        }
        break;
    case ANT_MAP_TYPE:
        if (initer->GetType() != ANT_INITER) {
            Error("You must init a map with an aggregate initializer !");
            return(false);
        } else {
            char errorstr[100];
            AstIniter *node = (AstIniter*)initer;
            AstMapType *map = (AstMapType*)type_spec;
            int array_size = node->elements_.size();
            int ii = 0;
            while (ii < array_size - 1) {
                if (!CheckIniter(map->key_type_, node->elements_[ii])) {
                    sprintf(errorstr, "The initializer #%d is not of the type of the map key", ii);
                    Error(errorstr);
                    return(false);
                }
                if (!CheckIniter(map->returned_type_, node->elements_[ii+1])) {
                    sprintf(errorstr, "The initializer #%d is not of the type of the map element", ii + 1);
                    Error(errorstr);
                    return(false);
                }
                ii += 2;
            }
            if (ii < array_size) {
                Error("The initializer of a map must have an even number of items (alternating keys and values)");
                return(false);
            }
        }
        break;
    }
    return(true);
}

bool AstChecker::CheckArrayIniter(AstArrayOrMatrixType *type_spec, int array_dimension, IAstNode *initer)
{
    bool success;

    if (initer->GetType() != ANT_INITER) {
        Error("You must init an aggregate with an aggregate initializer !");
        return(false);
    }
    AstIniter *node = (AstIniter*)initer;
    int array_size = type_spec->dimensions_[array_dimension];
    if (array_size == 0) {
        array_size = node->elements_.size();
        if (!type_spec->is_matrix_) {
            type_spec->dimensions_[array_dimension] = array_size;
        }
    }
    int to_check = node->elements_.size();
    if (array_size < to_check) {
        to_check = array_size;
        Error("Too many initializers");
        success = false;
    }
    if (array_dimension + 1 < type_spec->dimensions_.size()) {
        for (int ii = 0; ii < to_check; ++ii) {
            if (!CheckArrayIniter(type_spec, array_dimension + 1, node->elements_[ii])) {
                success = false;
                break;
            }
        }
    } else if (node->elements_[0]->GetType() == ANT_INITER) {
        for (int ii = 0; ii < to_check; ++ii) {
            if (!CheckIniter(type_spec->element_type_, node->elements_[ii])) {
                success = false;
                break;
            }
        }
    } else {
        ExpressionAttributes dst_attr;

        // we could just call CheckIniter but this way it is faster (we don't compute dst_attr all the times)
        dst_attr.InitWithTree(type_spec->element_type_, true, this);
        if (dst_attr.IsOnError()) {
            success = false;
        }
        for (int ii = 0; ii < to_check && success; ++ii) {
            if (initer->GetType() == ANT_INITER) {
                Error("Only a single value is allowed here (not an aggregate initializer)");
                success = false;
            } else {
                ExpressionAttributes src_attr;

                CheckExpression((IAstExpNode*)initer, &src_attr);
                if (src_attr.IsOnError()) {
                    success = false;
                } else {
                    success = CanAssign(&dst_attr, &src_attr);
                }
            }
        }
    }
    return(success);
}

void AstChecker::CheckBlock(AstBlock *block)
{
    string  text;
    int     ii;

    for (ii = 0; ii < block->block_items_.size(); ++ii) {
        IAstNode *node = block->block_items_[ii];
        switch (node->GetType()) {
        case ANT_VAR:
            CheckVar((VarDeclaration*)node);
            break;
        case ANT_CONST:
            CheckConst((ConstDeclaration*)node);
            break;
        case ANT_ASSIGNMENT:
            CheckAssignment((AstAssignment*)node);
            break;
        case ANT_UPDATE:
            CheckUpdateStatement((AstUpdate*)node);
            break;
        case ANT_INCDEC:
            CheckIncDec((AstIncDec*)node);
            break;
        case ANT_WHILE:
            CheckWhile((AstWhile*)node);
            break;
        case ANT_IF:
            CheckIf((AstIf*)node);
            break;
        case ANT_FOR:
            CheckFor((AstFor*)node);
            break;
        case ANT_SIMPLE:
            CheckSimpleStatement((AstSimpleStatement*)node);
            break;
        case ANT_RETURN:
            CheckReturn((AstReturn*)node);
            break;
        }
    }
}

void AstChecker::CheckAssignment(AstAssignment *node)
{
    int ii;

    for (ii = 0; ii < node->left_terms_.size(); ++ii) {
        ExpressionAttributes attr_left, attr_right;

        CheckExpression(node->left_terms_[ii], &attr_left);
        CheckExpression(node->right_terms_[ii], &attr_right);
        if (!attr_left.IsOnError() && !attr_right.IsOnError()) {
            CanAssign(&attr_left, &attr_right);
        }
    }
}

void AstChecker::CheckUpdateStatement(AstUpdate *node)
{
    ExpressionAttributes attr_left, attr_right, attr_src;
    string  text;

    CheckExpression(node->left_term_, &attr_left);
    CheckExpression(node->right_term_, &attr_right);
    if (!attr_left.IsOnError() && !attr_right.IsOnError()) {
        attr_src = attr_left;
        if (!attr_src.UpdateWithBinopOperation(&attr_right, node->operation_, &text)) {
            Error(text.c_str());
        } else {
            CanAssign(&attr_left, &attr_right);
        }
    }
}

void AstChecker::CheckIncDec(AstIncDec *node)
{
    ExpressionAttributes attr;

    CheckExpression(node->left_term_, &attr);
    if (!attr.IsOnError() && !attr.CanIncrement()) {
        Error("Increment and Decrement operations can be applied only to integer variables");
    }
}

void AstChecker::CheckWhile(AstWhile *node)
{
    ExpressionAttributes attr;

    CheckExpression(node->expression_, &attr);
    if (!attr.IsOnError() && !attr.IsBool()) {
        Error("Expression must evaluate to a bool");
    }
    ++loops_nesting;
    CheckBlock(node->block_);
    --loops_nesting;
}

void AstChecker::CheckIf(AstIf *node)
{
    int     ii;

    for (ii = 1; ii < node->expressions_.size(); ++ii) {
        ExpressionAttributes attr;
        CheckExpression(node->expressions_[ii], &attr);
        if (!attr.IsOnError() && !attr.IsBool()) {
            Error("Expression must evaluate to a bool");
        }
        CheckBlock(node->blocks_[ii]);
    }
    if (node->default_block_ != NULL) {
        CheckBlock(node->default_block_);
    }
}

void AstChecker::CheckFor(AstFor *node)
{
}

/*
string      index_name_;
string      iterator_name_;
IAstNode    *set_;
IAstNode    *low_;
IAstNode    *high_;
IAstNode    *step_;
AstBlock    *block_;
*/

void AstChecker::CheckSimpleStatement(AstSimpleStatement *node)
{
    if (loops_nesting == 0) {
        Error("You can only break/continue a for or while loop.");
    }
}

void AstChecker::CheckReturn(AstReturn *node)
{
    if (node->retvalue_ == NULL) {
        if (!return_fake_variable_.IsOnError() && !return_fake_variable_.IsVoid()) {
            Error("You must return a value when you return from a non-void function.");
        }
    } else {
        ExpressionAttributes    attr;

        CheckExpression(node->retvalue_, &attr);
        CanAssign(&return_fake_variable_, &attr);     // don't need to check - it emits the necessary messages.
    }
}

void AstChecker::CheckExpression(IAstExpNode *node, ExpressionAttributes *attr)
{
    int priority = 0;

    // check: return type and costness.

    switch (node->GetType()) {
    case ANT_INDEXING:
        CheckIndices((AstIndexing*)node, attr);
        priority = 2;
        break;
    case ANT_FUNCALL:
        CheckFunCall((AstFunCall*)node, attr);
        priority = 2;
        break;
    case ANT_BINOP:
        CheckBinop((AstBinop*)node, attr);
        break;
    case ANT_UNOP:
        CheckUnop((AstUnop*)node, attr);
        break;
    case ANT_EXP_LEAF:
        CheckLeaf((AstExpressionLeaf*)node, attr);
        break;
    }
}

void AstChecker::CheckIndices(AstIndexing *node, ExpressionAttributes *attr)
{
    bool        failure;
    int         ii, jj;
    string      error;
    IAstExpNode *exp;
    vector<ExpressionAttributes>    attributes;

    CheckExpression(node->left_term_, attr);
    failure = attr->IsOnError();
    if (node->lower_values_.size() > 0) {
        attributes.reserve(node->lower_values_.size() << 1);
        jj = 0;
        for (int ii = 0; ii < node->lower_values_.size(); ++ii) {
            exp = node->lower_values_[ii];
            if (exp != NULL) {
                CheckExpression(exp, &attributes[jj]);
                failure = failure || attributes[jj].IsOnError();
            }
            ++jj;
            exp = node->upper_values_[ii];
            if (exp != NULL) {
                CheckExpression(exp, &attributes[jj]);
                failure = failure || attributes[jj].IsOnError();
            }
            ++jj;
        }
    }
    if (!failure) {
        if (attr->UpdateWithIndexing(&attributes, node, this, &error)) {
            Error(error.c_str());
        }
    } else {
        attr->SetError();
    }
}

void AstChecker::CheckFunCall(AstFunCall *node, ExpressionAttributes *attr)
{
    bool                            failure;
    vector<ExpressionAttributes>    attributes;
    string                          error;

    CheckExpression(node->left_term_, attr);
    failure = attr->IsOnError();
    if (node->arguments_.size() > 0) {
        attributes.reserve(node->arguments_.size());
        for (int ii = 0; ii < node->arguments_.size(); ++ii) {
            AstArgument *arg = node->arguments_[ii];
            if (arg != NULL) {
                CheckExpression(arg->expression_, &attributes[ii]);
                failure = failure || attributes[ii].IsOnError();
            }
        }
    }
    if (!failure) {
        if (attr->UpdateWithFunCall(&attributes, &node->arguments_, this, &error)) {
            Error(error.c_str());
        }
    } else {
        attr->SetError();
    }
}

void AstChecker::CheckBinop(AstBinop *node, ExpressionAttributes *attr)
{
    ExpressionAttributes attr_right;
    string               error;

    CheckExpression(node->operand_left_, attr);
    CheckExpression(node->operand_right_, &attr_right);
    switch (node->subtype_) {
    case TOKEN_POWER:
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_MOD:
    case TOKEN_SHR:
    case TOKEN_SHL:
    case TOKEN_AND:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_OR:
    case TOKEN_XOR:
        if (!attr->UpdateWithBinopOperation(&attr_right, node->subtype_, &error)) {
            Error(error.c_str());
        }
        break;
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_GTE:
    case TOKEN_LTE:
    case TOKEN_DIFFERENT:
    case TOKEN_EQUAL:
        if (!attr->UpdateWithRelationalOperation(&attr_right, node->subtype_, &error)) {
            Error(error.c_str());
        }
        break;
    case TOKEN_LOGICAL_AND:
    case TOKEN_LOGICAL_OR:
        if (!attr->UpdateWithBoolOperation(&attr_right, &error)) {
            Error(error.c_str());
        }
        break;
    default:
        assert(false);
        break;
    }
}

void AstChecker::CheckUnop(AstUnop *node, ExpressionAttributes *attr)
{
    CheckExpression(node->operand_, attr);
    if (attr->IsOnError()) return;
    if (node->subtype_ == TOKEN_AND) {
        if (!IsALocalVariable(node->operand_) || !attr->TakeAddress()) {
            Error("You can apply the & unary operator only to a local variable.");
            attr->SetError();
        }
        return;
    }
    string error;
    if (!attr->UpdateWithUnaryOperation(node->subtype_, this, &error)) {
        Error(error.c_str());
    }
}

void AstChecker::CheckLeaf(AstExpressionLeaf *node, ExpressionAttributes *attr)
{
    if (node->subtype_ == TOKEN_NAME) {
        IAstDeclarationNode *decl;
        
        decl = FindDeclaration(node->value_.c_str());
        if (decl != NULL) {
            if (FailIfTypeDeclaration(decl)) {
                attr->SetError();
                return;
            }
            node->wp_type = decl->GetSingType();
            attr->InitWithTree(node->wp_type, decl->GetType() == ANT_VAR, this);
        } else {
            if (FindNamespace(node->value_.c_str())) {

            } else {
                Error("Undefined symbol");
            }
        }
    } else {
        attr->InitWithLiteral(node->subtype_, node->value_.c_str());
        node->ctc_ = true;
    }
}

void AstChecker::InsertName(const char *name, IAstDeclarationNode *declaration)
{
    if (!symbols_.InsertName(name, declaration)) {
        DupSymbolError(symbols_.FindDeclaration(name), declaration);
    }
}

IAstTypeNode *AstChecker::TypeFromTypeName(const char *name)
{
    IAstDeclarationNode *decl = symbols_.FindGlobalDeclaration(name);
    if (decl != NULL && decl->GetType() == ANT_TYPE) {
        return(((TypeDeclaration*)decl)->type_spec_);
    }

    // TODO: here you will need to search forward class and interface declarations

    return(NULL);
}

IAstDeclarationNode *AstChecker::FindDeclaration(const char *name, bool search_forward)
{
    IAstDeclarationNode *node = symbols_.FindDeclaration(name);
    if (node != NULL) return(node);
    if (search_forward) {
        for (int ii = current_ + 1; ii < root_->declarations_.size(); ++ii) {
            node = root_->declarations_[ii];
            switch (node->GetType()) {
            case ANT_FUNC:
                //case ANT_CLASS:
                //case ANT_INTERFACE:
                return(node);
            }
        }
    }
    return(NULL);
}

bool AstChecker::FailIfTypeDeclaration(IAstNode *node)
{
    switch (node->GetType()) {
    case ANT_TYPE:
    //case ANT_CLASS:
    //case ANT_INTERFACE:
    //case ANT_ENUM:
        Error("A type name found where a variable/const/function was required");
        return(false);
    default:
        break;
    }
    return(true);
}

bool AstChecker::FailIfNotTypeDeclaration(IAstNode *node)
{
    switch (node->GetType()) {
    case ANT_TYPE:
    //case ANT_CLASS:
    //case ANT_INTERFACE:
    //case ANT_ENUM:
        return(true);
    default:
        Error("A type name is required here");
    }
    return(false);
}

bool AstChecker::FindNamespace(const char *name)
{
    return(false);  // for now !!
}

bool AstChecker::IsALocalVariable(IAstExpNode *node)
{
    if (node != NULL && node->GetType() == ANT_EXP_LEAF) {
        AstExpressionLeaf *leaf = (AstExpressionLeaf*)node;
        if (leaf->subtype_ == TOKEN_NAME) {
            IAstDeclarationNode *decl = symbols_.FindLocalDeclaration(leaf->value_.c_str());
            if (decl != NULL) {
                AstNodeType nodetype = decl->GetType();
                if (nodetype == ANT_VAR) {
                    ((VarDeclaration*)decl)->pointed_ = true;
                    return(true);
                } else if (nodetype == ANT_CONST) {
                    ((ConstDeclaration*)decl)->pointed_ = true;
                    return(true);
                }
            }
        }
    }
    return(false);
}

bool AstChecker::CanAssign(ExpressionAttributes *dst, ExpressionAttributes *src)
{
    string error;

    if (!dst->CanAssign(src, this, &error)) {
        Error(error.c_str());
        return(false);
    }
    return(true);
}

bool AstChecker::CheckArrayIndices(AstArrayOrMatrixType *array)
{
    bool success = true;

    if (array->dimensions_.size() != array->expressions_.size()) {
        array->dimensions_.clear();
        for (int ii = 0; ii < array->expressions_.size(); ++ii) {
            IAstExpNode *exp = array->expressions_[ii];
            size_t      value = 0;
            
            if (exp != NULL) {
                ExpressionAttributes attr;

                CheckExpression(exp, &attr);
                if (!attr.IsAValidArraySize(&value)) {
                    Error("Array size must be '*' (for dynamic) or a positive integer value");
                    value = 0;
                    success = false;
                }
            }
            array->dimensions_.push_back(value);
        }
    }
    return(success);
}

bool AstChecker::CompareTypeTrees(IAstTypeNode *dst_tree, IAstTypeNode *src_tree)
{

}

IAstTypeNode *AstChecker::SolveTypedefs(IAstTypeNode *begin)
{
    if (begin == NULL) return(NULL);
    for (;;) {
        switch (begin->GetType()) {
        case ANT_QUALIFIED_TYPE:
            // TODO
            return(NULL);
        case ANT_NAMED_TYPE:
            const char *name = ((AstNamedType*)begin)->name_.c_str();
            begin = TypeFromTypeName(name);
            break;
        default:
            return(begin);
        }
    }
}

bool AstChecker::CompareTypesForAssignment(IAstTypeNode *dst, IAstTypeNode *src)
{

}

void AstChecker::DupSymbolError(IAstNode *old_declaration, IAstNode *new_declaration)
{
    //string errmess = "Duplicated symbol ";

    //errmess += 
}

void AstChecker::Error(const char *message)
{

}


} // namespace