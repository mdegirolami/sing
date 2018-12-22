#include <assert.h>
#include "expression_attributes.h"

namespace SingNames {

ExpBaseTypes Token2ExpBaseTypes(Token token)
{
    ExpBaseTypes    val = BT_TREE;
    switch (token) {
    case TOKEN_INT8: val = BT_INT8; break;
    case TOKEN_INT16: val = BT_INT16; break;
    case TOKEN_INT32: val = BT_INT32; break;
    case TOKEN_INT64: val = BT_INT64; break;
    case TOKEN_UINT8: val = BT_UINT8; break;
    case TOKEN_UINT16: val = BT_UINT16; break;
    case TOKEN_UINT32: val = BT_UINT32; break;
    case TOKEN_UINT64: val = BT_UINT64; break;
    case TOKEN_FLOAT32: val = BT_FLOAT32; break;
    case TOKEN_FLOAT64: val = BT_FLOAT64; break;
    case TOKEN_COMPLEX64: val = BT_COMPLEX64; break;
    case TOKEN_COMPLEX128: val = BT_COMPLEX128; break;
    case TOKEN_STRING: val = BT_STRING; break;
    case TOKEN_RUNE: val = BT_RUNE; break;
    case TOKEN_BOOL: val = BT_BOOL; break;
    case TOKEN_SIZE_T: val = BT_SIZE_T; break;
    case TOKEN_VOID: val = BT_VOID; break;
    default: break;
    }
    assert(val != BT_TREE);
    return(val);
}

void ExpressionAttributes::InitWithTree(IAstTypeNode *tree, bool is_a_variable, ITypedefSolver *solver)
{
    exp_type_ = BT_TREE;
    type_tree_ = tree;
    is_a_left_value_ = is_a_variable;
    Normalize(solver);
}

void ExpressionAttributes::InitWithLiteral(Token literal_type, const char *value)
{
    switch (literal_type) {
    case TOKEN_NULL:
        exp_type_ = BT_LITERAL_NULL;
        break;
    case TOKEN_FALSE:
    case TOKEN_TRUE:
        exp_type_ = BT_BOOL;
        break;
    case TOKEN_LITERAL_STRING:
        exp_type_ = BT_LITERAL_STRING;
        break;
    case TOKEN_LITERAL_UINT:
        exp_type_ = BT_LITERAL_NUMBER;
        value_.InitFromUnsignedString(value);
        break;
    case TOKEN_LITERAL_FLOAT:
        exp_type_ = BT_LITERAL_NUMBER;
        value_.InitFromFloatString(value);
        break;
    case TOKEN_LITERAL_IMG:
        exp_type_ = BT_LITERAL_NUMBER;
        value_.InitImgPartFromFloatString(value);
        break;
    }
    is_a_left_value_ = false;
}

bool ExpressionAttributes::ApplyTheIndirectionOperator(ITypedefSolver *solver)
{
    is_a_left_value_ = true;         // if I took its address, it is a local variable.
    
    // must be an address
    if (exp_type_ == BT_ADDRESS_OF) {
        exp_type_ = exp_pointed_type_;
        return(true);
    }

    // or a pointer
    if (exp_type_ != BT_TREE || type_tree_ == NULL || type_tree_->GetType() != ANT_POINTER_TYPE) {
        return(false);
    }
    IAstTypeNode *newtree = ((AstPointerType*)type_tree_)->pointed_type_;
    if (newtree == NULL) return(false);
    type_tree_ = newtree;
    Normalize(solver);
    return(true);
}

void ExpressionAttributes::Normalize(ITypedefSolver *solver)
{
    while (exp_type_ == BT_TREE) {
        AstNodeType nodetype = type_tree_->GetType();
        if (nodetype == ANT_QUALIFIED_TYPE) {
            break;
            // TO DO
        } else if (nodetype == ANT_NAMED_TYPE) {
            const char *name = ((AstNamedType*)type_tree_)->name_.c_str();
            IAstTypeNode *newtree = solver->TypeFromTypeName(name);
            if (newtree == NULL) {
                exp_type_ = BT_ERROR;
            } else {
                type_tree_ = newtree;
            }
        } else if (nodetype == ANT_BASE_TYPE) {
            exp_type_ = Token2ExpBaseTypes(((AstBaseType*)type_tree_)->base_type_);
        } else {
            // is a complex declaration - can't be reduced to an enum.
            break;
        }
    }
}

bool ExpressionAttributes::UpdateWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error)
{
    is_a_left_value_ = false;         // is a temporary value

    if (exp_type_ == BT_ERROR || attr_right->exp_type_ == BT_ERROR) {
        exp_type_ = BT_ERROR;
        return(true);           // silent. An error has been emitted yet.
    }

    ExpBaseTypes right_type = attr_right->exp_type_;

    // string-char sums. The output of a string (literal or not) and any unsigned integer ni the 0..0x3fffff range is a string
    if (operation == TOKEN_PLUS) {
        if ((exp_type_ & BT_STRINGS_AND_LITERALS) != 0 || (right_type & BT_STRINGS_AND_LITERALS) != 0) {
            bool valid_literal0 = exp_type_ == BT_LITERAL_STRING || exp_type_ == BT_LITERAL_NUMBER && value_.IsAValidCharValue();
            bool valid0 = valid_literal0 || (exp_type_ & (BT_STRING + BT_UINT8 + BT_UINT16 + BT_UINT32 + BT_RUNE)) != 0;
            bool valid_literal1 = right_type == BT_LITERAL_STRING || right_type == BT_LITERAL_NUMBER && attr_right->value_.IsAValidCharValue();
            bool valid1 = valid_literal1 || (right_type & (BT_STRING + BT_UINT8 + BT_UINT16 + BT_UINT32 + BT_RUNE)) != 0;
            if (valid0 && valid1) {
                if (valid_literal0 && valid_literal1) {
                    exp_type_ = BT_LITERAL_STRING;
                } else {
                    exp_type_ = BT_STRING;
                }
                return(true);
            }
        }
    }

    if ((exp_type_ & BT_NUMBERS_AND_LITERALS) == 0 ||
        (right_type & BT_NUMBERS_AND_LITERALS) == 0) {
        if (operation == TOKEN_PLUS) {
            *error = "Operands must be numbers or strings, you can add strings to unsigned in the 0..0x3fffff range";
        } else {
            *error = "Operands must be numbers";
        }
        exp_type_ = BT_ERROR;
        return(false);
    }
    bool left_is_literal = (exp_type_ & BT_LITERAL_NUMBER) != 0;
    bool right_is_literal = (right_type & BT_LITERAL_NUMBER) != 0;
    if (left_is_literal) {
        if (right_is_literal) {
            switch (value_.PerformOp(&attr_right->value_, operation)) {
            case NumericValue::OE_ILLEGAL_OP:
                *error = "Operation is not compatible with the operand's types";
                exp_type_ = BT_ERROR;
                break;
            case NumericValue::OE_INTEGER_POWER_WRONG:
                *error = "On an integer base you can only use an integer exponent ranging 0 to 63 (but not 0^0)";
                exp_type_ = BT_ERROR;
                break;
            case NumericValue::OE_OVERFLOW:
                *error = "Operation is not compatible with the operand's types";
                exp_type_ = BT_ERROR;
                break;
            case NumericValue::OE_INTEGER_OVERFLOW:
                *error = "Operation causes an integer overflow, consider using floating point literals";
                exp_type_ = BT_ERROR;
                break;
            case NumericValue::OE_SHIFT_TOO_BIG:
                *error = "Shifting more than 64 positions has no purpose (you get 0 or -1)";
                exp_type_ = BT_ERROR;
                break;
            case NumericValue::OE_NAN_RESULT:
                *error = "Operation result is undefined";
                exp_type_ = BT_ERROR;
                break;
            default:
                return(true);
            }
        } else {
            if (!IsValueCompatible(right_type)) {
                *error = "Left literal operator doesn't fit the range of the right operand type.";
                exp_type_ = BT_ERROR;
            } else if ((right_type & (BT_ALL_FLOATS | BT_ALL_COMPLEX)) != 0 && !OperationSupportsFloatingPoint(operation)) {
                // note: if left fits right and right is not a float, left must have an integer value.
                *error = "Operation requires integer operands";
                exp_type_ = BT_ERROR;
            } else {
                exp_type_ = right_type;
                return(true);
            }
        }
    } else {
        if (right_is_literal) {
            if (!attr_right->IsValueCompatible(exp_type_)) {
                *error = "Right literal operator doesn't fit the range of the left operand type.";
                exp_type_ = BT_ERROR;
            } else if ((exp_type_ & (BT_ALL_FLOATS | BT_ALL_COMPLEX)) != 0 && !OperationSupportsFloatingPoint(operation)) {
                // note: if right fits left and left is not a float, right must have an integer value.
                *error = "Operation requires integer operands";
                exp_type_ = BT_ERROR;
            } else {
                return(true);
            }
        } else {
            exp_type_ = CanPromote(exp_type_, right_type);
            if (exp_type_ == BT_ERROR) {
                *error = "Operators Mismatch, explicit cast required.";
            } else  if ((exp_type_ & (BT_ALL_FLOATS | BT_ALL_COMPLEX)) != 0 && !OperationSupportsFloatingPoint(operation)) {
                *error = "Operation requires integer operands";
                exp_type_ = BT_ERROR;
            } else {
                return(true);
            }
        }
    }
    return(false);
}

// for general operations (binops) types smaller than 32 bits are automatically promoted
bool ExpressionAttributes::IsValueCompatible(ExpBaseTypes the_type)
{
    switch (the_type) {
    case BT_COMPLEX128:
        return(true);
    case BT_COMPLEX64:
        return(value_.FitsComplex64());
    case BT_FLOAT64:
        return(value_.FitsFloat(true));
    case BT_FLOAT32:
        return(value_.FitsFloat(false));
    case BT_INT8:
    case BT_INT16:
    case BT_INT32:
        return(value_.FitsSigned(32));
    case BT_INT64:
        return(value_.FitsSigned(64));
    case BT_UINT8:
    case BT_UINT16:
    case BT_UINT32:
    case BT_RUNE:
        return(value_.FitsUnsigned(32));
    case BT_UINT64:
    case BT_SIZE_T:
        return(value_.FitsUnsigned(64));
    default:
        break;
    }
    return(false);
}

bool ExpressionAttributes::OperationSupportsFloatingPoint(Token operation)
{
    switch (operation) {
    case TOKEN_POWER:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
        return(true);
    }
    return(false);
}

// returns the result type
ExpBaseTypes ExpressionAttributes::CanPromote(ExpBaseTypes op1, ExpBaseTypes op2)
{
    int32_t num_bits1, num_bits2, num_exp_bits1, num_exp_bits2;
    bool    has_sign1, has_sign2, is_complex1, is_complex2;

    GetBaseTypePrecision(op1, &num_bits1, &num_exp_bits1, &has_sign1, &is_complex1);
    GetBaseTypePrecision(op2, &num_bits2, &num_exp_bits2, &has_sign2, &is_complex2);

    // can fit int32 ?
    if (!is_complex1 && !is_complex2 && num_exp_bits1 == 0 && num_exp_bits2 == 0) {
        if ((has_sign1 || has_sign2) && num_bits1 <= 31 && num_bits2 <= 31) {
            return(BT_INT32);
        } else if (!has_sign1 && !has_sign2 && num_bits1 <= 32 && num_bits2 <= 32) {
            return(BT_UINT32);
        }
    }

    // promote first to second or viceversa
    if ((is_complex1 || !is_complex2) && (has_sign1 || !has_sign2) && num_bits1 >= num_bits2 && num_exp_bits1 >= num_exp_bits2) {
        return(op1);
    } else if ((is_complex2 || !is_complex1) && (has_sign2 || !has_sign1) && num_bits2 >= num_bits1 && num_exp_bits2 >= num_exp_bits1) {
        return(op2);
    }
    return(BT_ERROR);
}

void ExpressionAttributes::GetBaseTypePrecision(ExpBaseTypes the_type, int32_t *num_bits, int32_t *num_exp_bits, bool *has_sign, bool *is_complex)
{
    // defaults
    *has_sign = (the_type & BT_ALL_UINTS) == 0;
    *is_complex = (the_type & BT_ALL_COMPLEX) != 0;
    *num_exp_bits = 0;
    if ((the_type & (BT_INT8 + BT_UINT8)) != 0) {
        *num_bits = 8;
    } else if ((the_type & (BT_INT16 + BT_UINT16)) != 0) {
        *num_bits = 16;
    } else if ((the_type & (BT_INT32 + BT_UINT32 + BT_RUNE)) != 0) {
        *num_bits = 32;
    } else if ((the_type & (BT_INT64 + BT_UINT64 + BT_SIZE_T)) != 0) {
        *num_bits = 64;
    } else if ((the_type & (BT_FLOAT32 + BT_COMPLEX64)) != 0) {
        *num_bits = 25;     // add 2: one is the implicit leadin 1, the other to compensate the decrement
        *num_exp_bits = 8;
    } else if ((the_type & (BT_FLOAT64 + BT_COMPLEX128)) != 0) {
        *num_bits = 54;     // add 2: one is the implicit leadin 1, the other to compensate the decrement
        *num_exp_bits = 11;
    }
    if (has_sign) *num_bits -= 1;
}

bool ExpressionAttributes::UpdateWithRelationalOperation(ExpressionAttributes *attr_right, Token operation, string *error)
{
    is_a_left_value_ = false;         // is a temporary value

    if (exp_type_ == BT_ERROR || attr_right->exp_type_ == BT_ERROR) {
        exp_type_ = BT_ERROR;
        return(true);           // silent. An error has been emitted yet.
    }
    if (IsComplex() || attr_right->IsComplex()) {
        *error = "Dont' know how to compare complex numbers";
        exp_type_ = BT_ERROR;
        return(false);
    }
    if ((exp_type_ & BT_NUMBERS_AND_LITERALS) != 0 && (attr_right->exp_type_ & BT_NUMBERS_AND_LITERALS) != 0) {
        exp_type_ = BT_BOOL;
        return(true);
    }
    if ((exp_type_ & BT_STRINGS_AND_LITERALS) != 0 && (attr_right->exp_type_ & BT_STRINGS_AND_LITERALS) != 0) {
        exp_type_ = BT_BOOL;
        return(true);
    }
    if (exp_type_ == BT_BOOL && attr_right->exp_type_ == BT_BOOL && (operation == TOKEN_EQUAL || operation == TOKEN_DIFFERENT)) {
        return(true);
    }
    *error = "You can only compare two numbers, strings or bools (the latter for equality only)";
    return(false);
}

bool ExpressionAttributes::IsComplex(void)
{
    return ((exp_type_ & BT_ALL_COMPLEX) != 0 || exp_type_ == BT_LITERAL_NUMBER && value_.IsComplex());
}

bool ExpressionAttributes::UpdateWithBoolOperation(ExpressionAttributes *attr_right, string *error)
{
    is_a_left_value_ = false;         // is a temporary value

    if (exp_type_ == BT_ERROR || attr_right->exp_type_ == BT_ERROR) {
        exp_type_ = BT_ERROR;
        return(true);           // silent. An error has been emitted yet.
    }
    if ((exp_type_ & attr_right->exp_type_ & BT_BOOL) != 0) {
        return(true);
    }
    *error = "You can apply '&&' and '||' only to bools";
    return(false);
}

bool ExpressionAttributes::UpdateWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error)
{
    is_a_left_value_ = false;         // is a temporary value - except for *, as fixed in ApplyTheIndirectionOperator()

    if (exp_type_ == BT_ERROR) {
        return(true);
    }
    if (operation == TOKEN_MPY) {
        if (!ApplyTheIndirectionOperator(solver)) {
            *error = "You can use unary * only on pointers and addresses";
            exp_type_ = BT_ERROR;
        }
        return(false);
    } else if (exp_type_ == BT_ADDRESS_OF) {
        *error = "On a pointer the only allowed operator is '*'";
        exp_type_ = BT_ERROR;
        return(false);
    }
    switch (operation) {
    case TOKEN_SIZEOF:
                                    // TODO
        break;
    case TOKEN_DIMOF:
        break;                      // TODO
    case TOKEN_MINUS:
        if ((exp_type_ & (BT_ALL_THE_NUMBERS - BT_ALL_UINTS + BT_LITERAL_NUMBER)) == 0) {
            *error = "Unary '-' only apply to numeric signed basic types";
            exp_type_ = BT_ERROR;
        } else if (exp_type_ == BT_LITERAL_NUMBER) {
            if (value_.PerformUnop(TOKEN_MINUS) == NumericValue::OE_INTEGER_OVERFLOW) {
                *error = "Number is too big to be stored as a negative integer. consider using a flot literal";
                exp_type_ = BT_ERROR;
            }
        }
        break;
    case TOKEN_PLUS:
        if ((exp_type_ & (BT_ALL_THE_NUMBERS - BT_ALL_UINTS + BT_LITERAL_NUMBER)) == 0) {
            *error = "Unary '+' only apply to numeric signed basic types";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_NOT:
        if ((exp_type_ & (BT_ALL_UINTS | BT_ALL_INTS | BT_LITERAL_NUMBER)) == 0) {
            *error = "The arithmetic not '~' operator requires an integer value";
            exp_type_ = BT_ERROR;
        } else if(exp_type_ == BT_LITERAL_NUMBER) {
            if (value_.PerformUnop(TOKEN_NOT) == NumericValue::OE_ILLEGAL_OP) {
                *error = "The arithmetic not '~' operator requires an integer value";
                exp_type_ = BT_ERROR;
            }
        }
        break;
    case TOKEN_LOGICAL_NOT:
        if ((exp_type_ & BT_BOOL) == 0) {
            *error = "The logical not (!) operator requires a boolean";
            exp_type_ = BT_ERROR;
        }
        break;

        // TYPE COVERSIONS
    case TOKEN_INT8:
    case TOKEN_INT16:
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT8:
    case TOKEN_UINT16:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
    case TOKEN_RUNE:
    case TOKEN_SIZE_T:
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        if ((exp_type_ & (BT_LITERAL_NUMBER | BT_LITERAL_STRING)) != 0) {
            *error = "Numeric conversions are unallowed on literals and literal expressions.";
            exp_type_ = BT_ERROR;
        } else if ((exp_type_ & (BT_ALL_THE_NUMBERS | BT_STRING)) != 0) {
            exp_type_ = Token2ExpBaseTypes(operation);
        } else {
            *error = "Unknown conversion.";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_STRING:
        if ((exp_type_ & (BT_NUMBERS_AND_LITERALS | BT_STRINGS_AND_LITERALS | BT_BOOL)) != 0) {
            exp_type_ = BT_STRING;
        } else {
            *error = "Unknown conversion.";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_BOOL:
        *error = "Please use relational operators to convert numbers and strings to bools.";
        exp_type_ = BT_ERROR;
        break;
    }
}

bool ExpressionAttributes::UpdateWithFunCall(vector<ExpressionAttributes> *attr_args, 
                                             vector<AstArgument*> *arg_descs, ITypedefSolver *solver, string *error)
{
    int     ii;
    char    errorbuf[128];

    if (exp_type_ == BT_ERROR) {
        return(true);
    }
    if (exp_type_ != BT_TREE || type_tree_ == NULL || type_tree_->GetType() != ANT_FUNC_TYPE) {
        *error = "You can apply an argument list only to an object of type 'function'";
        exp_type_ = BT_ERROR;
        return(false);
    }

    // in case of early exit
    exp_type_ = BT_ERROR;

    AstFuncType *typedesc = (AstFuncType*)type_tree_;
    for (ii = 0; ii < arg_descs->size() && ii < typedesc->arguments_.size(); ++ii) {
        AstArgument *argvalue = (*arg_descs)[ii];
        AstArgumentDecl *argdecl = typedesc->arguments_[ii];
        ExpressionAttributes *argvalue_attr = &(*attr_args)[ii];    // of actial argument expressions
        ExpressionAttributes argdecl_attr;                          // of a fake expression where we pretend the formal argument is a variable.

        if (argvalue == NULL) {
            if (argdecl->initer_ == NULL) {
                sprintf(errorbuf, "Argument %d is not specified and has no default value", ii);
                *error = errorbuf;
                return(false);
            }
            continue;
        }

        // if we are here argvalue != NULL and argvalue_attr is inited.
        if (argvalue_attr->exp_type_ == BT_ERROR) {
            return(false);
        }

        // check tags
        if (argvalue->name_[0] != 0 && argvalue->name_ != argdecl->name_) {
            sprintf(errorbuf, "Argument %d has mismatching name", ii);
            *error = errorbuf;
            return(false);
        }

        // check types compatibility
        argdecl_attr.InitWithTree(argdecl->type_, true, solver);
        if (argdecl->direction_ == PD_ABSENT) {

            // dont' allow undefined direction for now.
            *error = "You need to specify a direction for an argument.";
            return(false);
        } else if (argdecl->direction_ == PD_IN) {
            if (!argdecl_attr.CanAssign(argvalue_attr, solver, error)) {
                return(false);
            }
        } else { 

            // out or io
            if (!argvalue_attr->is_a_left_value_) {
                sprintf(errorbuf, "Argument %d is an output argument and needs to be an assignable variable", ii);
                *error = errorbuf;
                return(false);
            }

            // type check !!
            /*
            bool typematch = false;
            if (argvalue_attr->exp_type_ == argdecl_attr.exp_type_) {
                if (argvalue_attr->exp_type_ == BT_TREE) {
                    typematch = AreTypeTreesCompatible(argvalue_attr->type_tree_, argdecl->type_, WITH_OUTPUT_DECLARATION, solver);
                } else {
                    typematch = true;
                }
            }
            */
            // because the tree is always valid, even on normalized stuff (except in case of error)
            bool typematch = AreTypeTreesCompatible(argvalue_attr->type_tree_, argdecl->type_, WITH_OUTPUT_DECLARATION, solver);
            if (!typematch) {
                sprintf(errorbuf, "Argument %d type mismatch", ii);
                *error = errorbuf;
                return(false);
            }
        }
    }
    InitWithTree(typedesc->return_type_, false, solver);
    return(true);
}

bool ExpressionAttributes::UpdateWithIndexing(vector<ExpressionAttributes> *attr_args, AstIndexing *node, ITypedefSolver *solver, string *error)
{
    int     ii;
    char    errorbuf[128];

    if (exp_type_ == BT_ERROR) {
        return(true);
    }
    if (exp_type_ != BT_TREE || type_tree_ == NULL || (type_tree_->GetType() != ANT_MAP_TYPE && type_tree_->GetType() != ANT_ARRAY_TYPE)) {
        *error = "You can apply an index list only to objects of type 'map' or 'array'";
        exp_type_ = BT_ERROR;
        return(false);
    }
    exp_type_ = BT_ERROR;
    if (type_tree_->GetType() == ANT_MAP_TYPE) {
        AstMapType *typedesc = (AstMapType*)type_tree_;
        ExpressionAttributes argdecl_attr;                          // of a fake expression where we pretend the map input is a variable.

        if (node->is_single_index.size() != 1 || !node->is_single_index.size[0]) {
            *error = "Maps accept a single index";
            return(false);
        }
        if (argdecl_attr.CanAssign(&(*attr_args)[0], solver, error)) {
            InitWithTree(typedesc->returned_type_, true, solver);
            return(true);
        }
    } else {
        AstArrayOrMatrixType *typedesc = (AstArrayOrMatrixType*)type_tree_;

        // let's make typedesc->dimensions_ valid
        solver->CheckArrayIndices(typedesc);

        // all the inited attr must be valid indices.
        // if dimensions_ is defined (>0) they must fall in the range.
        for (ii = 0; ii < attr_args->size(); ++ii) {
            ExpressionAttributes *cur_attr = &(*attr_args)[ii];
            size_t  index_value;
            if (!cur_attr->IsOnError()) {   // uninited descriptors are marked as on error !
                if (!cur_attr->IsAValidArrayIndex(&index_value)) {
                    *error = "Indices must be positive integers";
                    return(false);
                } else {
                    size_t array_size = typedesc->dimensions_[ii >> 1];
                    if (array_size > 0 && index_value >= array_size) {
                        *error = "Index value is out of bound";
                        return(false);
                    }
                }
            }
        }
        InitWithTree(typedesc->element_type_, true, solver);
        return(true);
    }
    return(false);
}

bool ExpressionAttributes::TakeAddress(void)
{
    if (exp_type_ == BT_ERROR) return(true);
    if ((exp_type_ & (BT_ALL_THE_NUMBERS | BT_STRING | BT_BOOL | BT_TREE)) == 0) {
        return(false);
    }
    exp_pointed_type_ = exp_type_;
    exp_type_ = BT_ADDRESS_OF;
    is_a_left_value_ = false;
    return(true);
}

bool ExpressionAttributes::CanAssign(ExpressionAttributes *src, ITypedefSolver *solver, string *error)
{
    if (exp_type_ == BT_ERROR || src->exp_type_ == BT_ERROR) {
        return(true); // be silent
    }
    if (!is_a_left_value_) {
        *error = "You can assign only a variable or a portion of variable";
        return(false);
    }
    bool can_assign = false;
    if ((exp_type_ & BT_ALL_THE_NUMBERS) != 0) {
        if ((src->exp_type_ & BT_ALL_THE_NUMBERS) != 0) {
            int32_t num_bits1, num_bits2, num_exp_bits1, num_exp_bits2;
            bool    has_sign1, has_sign2, is_complex1, is_complex2;

            GetBaseTypePrecision(     exp_type_, &num_bits1, &num_exp_bits1, &has_sign1, &is_complex1);
            GetBaseTypePrecision(src->exp_type_, &num_bits2, &num_exp_bits2, &has_sign2, &is_complex2);
            can_assign = (is_complex1 || !is_complex2) && (has_sign1 || !has_sign2) &&
                         num_bits1 >= num_bits2 && num_exp_bits1 >= num_exp_bits2;
            if (!can_assign) {
                *error = "Assignment would cause loss of precision or range. Please cast explicitly";
            }
        } else if ((src->exp_type_ & BT_LITERAL_NUMBER) != 0) {
            can_assign = src->IsValueCompatibleForAssignment(exp_type_);
            if (!can_assign) {
                *error = "Value doesn't fit the destination";
            }
        } else {
            can_assign = false;
        }
    } else if (exp_type_ == BT_STRING) {
        can_assign = (src->exp_type_ & (BT_STRING + BT_LITERAL_STRING + BT_UINT8 + BT_UINT16 + BT_UINT32 + BT_RUNE)) != 0 ||
                     src->exp_type_ == BT_LITERAL_NUMBER && src->value_.IsAValidCharValue();
        if (!can_assign) {
            *error = "You can assign to a string another string or an unsigned integer max 32 bits in the 0..0x3fffff range";
        }
    } else if (exp_type_ == BT_BOOL) {
        can_assign = src->exp_type_ == BT_BOOL;
        if (!can_assign) {
            *error = "You can assign a bool only with a bool (use relational operators to generate bools)";
        }
    } else if (exp_type_ == BT_TREE) {
        if (src->exp_type_ == BT_ADDRESS_OF) {
            IAstTypeNode *dst_tree = SolveTypedefs(type_tree_, solver);
            if (dst_tree->GetType() == ANT_POINTER_TYPE) {
                AstPointerType *ptr_desc = (AstPointerType*)dst_tree;
                can_assign = AreTypeTreesCompatible(src->type_tree_, ptr_desc->pointed_type_, FOR_EQUALITY, solver);
            } else {
                can_assign = false;
            }
        } else if (src->exp_type_ == BT_TREE) {
            can_assign = AreTypeTreesCompatible(type_tree_, src->type_tree_, FOR_ASSIGNMENT, solver);
        } else {
            can_assign = false;
        }
    }
    if (!can_assign && *error == "") {
        *error = "No known conversion, can't perform assignment/update";
    }
    return(can_assign);
}

bool ExpressionAttributes::IsValueCompatibleForAssignment(ExpBaseTypes the_type)
{
    switch (the_type) {
    case BT_COMPLEX128:
        return(true);
    case BT_COMPLEX64:
        return(value_.FitsComplex64());
    case BT_FLOAT64:
        return(value_.FitsFloat(true));
    case BT_FLOAT32:
        return(value_.FitsFloat(false));
    case BT_INT8:
        return(value_.FitsSigned(8));
    case BT_INT16:
        return(value_.FitsSigned(16));
    case BT_INT32:
        return(value_.FitsSigned(32));
    case BT_INT64:
        return(value_.FitsSigned(64));
    case BT_UINT8:
        return(value_.FitsUnsigned(8));
    case BT_UINT16:
        return(value_.FitsUnsigned(16));
    case BT_UINT32:
    case BT_RUNE:
        return(value_.FitsUnsigned(32));
    case BT_UINT64:
    case BT_SIZE_T:
        return(value_.FitsUnsigned(64));
    default:
    }
    return(false);
}

IAstTypeNode *ExpressionAttributes::SolveTypedefs(IAstTypeNode *begin, ITypedefSolver *solver)
{
    if (begin == NULL) return(NULL);
    for (;;) {
        switch (begin->GetType()) {
        case ANT_QUALIFIED_TYPE:
            // TODO
            return(NULL);
        case ANT_NAMED_TYPE:
            const char *name = ((AstNamedType*)begin)->name_.c_str();
            begin = solver->TypeFromTypeName(name);
            break;
        default:
            return(begin);
        }
    }
}

bool ExpressionAttributes::AreTypeTreesCompatible(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode, ITypedefSolver *solver)
{
    IAstTypeNode            *t0, *src_tree;
    AstArrayOrMatrixType    *array0;
    AstArrayOrMatrixType    *array1;

    // must examine the trees
    t0 = SolveTypedefs(t0, solver);
    t1 = SolveTypedefs(t1, solver);
    if (t0 == NULL || t1 == NULL) {
        return(false);
    }
    AstNodeType t0type = t0->GetType();
    AstNodeType t1type = t1->GetType();
    if (t0type != t1type) return(false);
    if (t0type == ANT_ARRAY_TYPE) {
        array0 = (AstArrayOrMatrixType*)t0;
        array1 = (AstArrayOrMatrixType*)t1;
        solver->CheckArrayIndices(array0);
        solver->CheckArrayIndices(array1);
    }
    if (!t0->IsCompatible(t1, mode)) {
        return(false);
    }
    switch (t0type) {
    case ANT_ARRAY_TYPE:
        return (AreTypeTreesCompatible(array0->element_type_, array1->element_type_, FOR_EQUALITY, solver));
    case ANT_MAP_TYPE:
        AstMapType *map0 = (AstMapType*)t0;
        AstMapType *map1 = (AstMapType*)t1;
        return(AreTypeTreesCompatible(map0->key_type_, map1->key_type_, FOR_EQUALITY, solver) &&
               AreTypeTreesCompatible(map0->returned_type_, map1->returned_type_, FOR_EQUALITY, solver));
    case ANT_POINTER_TYPE:
        return (AreTypeTreesCompatible(((AstPointerType*)t0)->pointed_type_, ((AstPointerType*)t1)->pointed_type_, FOR_EQUALITY, solver));
    case ANT_FUNC_TYPE:
        AstFuncType *func0 = (AstFuncType*)t0;
        AstFuncType *func1 = (AstFuncType*)t1;
        if (!AreTypeTreesCompatible(func0->return_type_, func1->return_type_, FOR_EQUALITY, solver)) {
            return(false);
        }
        for (int ii = 0; ii < func0->arguments_.size(); ++ii) {
            AstArgumentDecl *arg0 = func0->arguments_[ii];
            AstArgumentDecl *arg1 = func1->arguments_[ii];
            if (!arg0->IsCompatible(arg1, FOR_EQUALITY)) {
                return(false);
            }
            if (!AreTypeTreesCompatible(arg0->type_, arg1->type_, FOR_EQUALITY, solver)) {
                return(false);
            }
        }
    default:
        break;
    }
    return(true);
}

bool ExpressionAttributes::IsAValidArrayIndex(size_t *value)
{
    *value = 0;
    if ((exp_type_ & (BT_ALL_UINTS + BT_ALL_INTS)) != 0) {
        return(true);
    }
    if (exp_type_ == BT_LITERAL_NUMBER && value_.IsAPositiveInteger()) {
        *value = value_.GetPositiveIntegerValue();
        return(true);
    }
    return(false);
}

// returns 0 if not known at run time
bool ExpressionAttributes::IsAValidArraySize(size_t *value)
{
    *value = 0;
    if ((exp_type_ & (BT_ALL_UINTS + BT_ALL_INTS)) != 0) {
        return(true);
    }
    if (exp_type_ == BT_LITERAL_NUMBER && value_.IsAPositiveInteger()) {
        *value = value_.GetPositiveIntegerValue();
        return(*value > 0);
    }
    return(false);
}

} // namespace