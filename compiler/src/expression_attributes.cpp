#include <assert.h>
#include "expression_attributes.h"
#include "ast_nodes.h"

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
    case TOKEN_BOOL: val = BT_BOOL; break;
    case TOKEN_VOID: val = BT_VOID; break;
    default: break;
    }
    assert(val != BT_TREE);
    return(val);
}

Token ExpBase2TokenTypes(ExpBaseTypes basetype)
{
    Token   val = TOKENS_COUNT;
    switch (basetype) {
    case BT_INT8: val = TOKEN_INT8; break;
    case BT_INT16: val = TOKEN_INT16; break;
    case BT_INT32: val = TOKEN_INT32; break;
    case BT_INT64: val = TOKEN_INT64; break;
    case BT_UINT8: val = TOKEN_UINT8; break;
    case BT_UINT16: val = TOKEN_UINT16; break;
    case BT_UINT32: val = TOKEN_UINT32; break;
    case BT_UINT64: val = TOKEN_UINT64; break;
    case BT_FLOAT32: val = TOKEN_FLOAT32; break;
    case BT_FLOAT64: val = TOKEN_FLOAT64; break;
    case BT_COMPLEX64: val = TOKEN_COMPLEX64; break;
    case BT_COMPLEX128: val = TOKEN_COMPLEX128; break;
    case BT_STRING: val = TOKEN_STRING; break;
    case BT_BOOL: val = TOKEN_BOOL; break;
    case BT_VOID: val = TOKEN_VOID; break;
    default: break;
    }
    return(val);
}

ExpressionAttributes::ExpressionAttributes()
{
    exp_type_ = exp_pointed_type_ = BT_ERROR;
    type_tree_ = original_tree_ = nullptr;
    is_writable_ = false;
    is_a_variable_ = false;
    value_is_valid_ = false;
    is_a_literal_ = false;
}

void ExpressionAttributes::InitWithTree(IAstTypeNode *tree, bool is_a_variable, bool is_writable, ITypedefSolver *solver)
{
    exp_type_ = tree == nullptr ? BT_ERROR : BT_TREE;
    type_tree_ = original_tree_ = tree;
    is_a_variable_ = is_a_variable;
    is_writable_ = is_writable;
    value_is_valid_ = is_a_literal_ = false;
    Normalize(solver);
}

void ExpressionAttributes::SetTheValueFrom(const ExpressionAttributes *attr)
{
    value_ = attr->value_;
    value_is_valid_ = attr->value_is_valid_;
}

void ExpressionAttributes::SetEnumValue(int32_t value)
{
    is_a_variable_ = is_writable_ = false;
    value_is_valid_ = is_a_literal_ = true;
    value_.InitFromInt32(value);
}

bool ExpressionAttributes::InitWithLiteral(Token literal_type, const char *value, const char *img_value, 
                                           bool isint, bool realneg, bool imgneg, string *error)
{
    value_is_valid_ = is_a_literal_ = false;
    switch (literal_type) {
    case TOKEN_NULL:
        exp_type_ = BT_LITERAL_NULL;
        break;
    case TOKEN_FALSE:
    case TOKEN_TRUE:
        exp_type_ = BT_BOOL;
        break;
    case TOKEN_LITERAL_STRING:
        exp_type_ = BT_STRING;
        break;
    case TOKEN_LITERAL_UINT:
        value_.InitFromUnsignedString(value);
        if (!value_.FitsSigned(32) || value_.PerformConversion(TOKEN_INT32) != NumericValue::OE_OK) {
            *error = "Value doesn't fit an int/i32. use the fake cast notation type(literal) es: i64(-0x1_0000_0000)";
            exp_type_ = BT_ERROR;
            return(false);
        }
        exp_type_ = BT_INT32;
        value_is_valid_ = is_a_literal_ = true;
        break;
    case TOKEN_LITERAL_FLOAT:
        value_.InitFromFloatString(value);
        if (!value_.FitsFloat(false) || value_.PerformConversion(TOKEN_FLOAT32) != NumericValue::OE_OK) {
            *error = "Value doesn't fit a float/f32. use the fake cast notation type(literal) es: f64(-1e50)";
            exp_type_ = BT_ERROR;
            return(false);
        }
        exp_type_ = BT_FLOAT32;
        value_is_valid_ = is_a_literal_ = true;
        break;
    case TOKEN_LITERAL_IMG:
        value_.InitImgPartFromFloatString(value);
        if (!value_.FitsComplex(false) || value_.PerformConversion(TOKEN_COMPLEX64) != NumericValue::OE_OK) {
            *error = "Value doesn't fit a complex64. use the fake cast notation type([real] + [img]) es: complex128(-1e50 + 5i)";
            exp_type_ = BT_ERROR;
            return(false);
        }
        exp_type_ = BT_COMPLEX64;
        value_is_valid_ = is_a_literal_ = true;
        break;
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        exp_type_ = Token2ExpBaseTypes(literal_type);
        if (!value_.InitFromStringsAndFlags(value, img_value, isint, realneg, imgneg)) {
            *error = "Value too big to be represented in an integer variable.";
            exp_type_ = BT_ERROR;
            return(false);
        }
        if (!IsValueCompatibleForAssignment(exp_type_) || value_.PerformConversion(literal_type) != NumericValue::OE_OK) {
            *error = "Value too big for the type or can't be represented in full precision.";
            exp_type_ = BT_ERROR;
            return(false);
        }
        value_is_valid_ = is_a_literal_ = true;
        break;
    }
    type_tree_ = original_tree_ = nullptr;
    is_a_variable_ = is_writable_ = false;
    return(true);
}

void ExpressionAttributes::InitWithInt32(int size)
{
    exp_type_ = BT_INT32;
    type_tree_ = original_tree_ = nullptr;
    is_a_variable_ = is_writable_ = false;
    value_is_valid_ = is_a_literal_ = true;
    value_.InitFromInt32(size);
}

bool ExpressionAttributes::ApplyTheIndirectionOperator(ITypedefSolver *solver)
{    
    // must be an address
    if (exp_type_ == BT_ADDRESS_OF) {
        is_a_variable_ = true;              // if I took its address, it is a local variable.
        exp_type_ = exp_pointed_type_;
        return(true);
    }

    // or a pointer
    if (exp_type_ != BT_TREE || type_tree_ == nullptr || type_tree_->GetType() != ANT_POINTER_TYPE) {
        return(false);
    }
    AstPointerType *pointer_decl = (AstPointerType*)type_tree_;
    IAstTypeNode *newtree = pointer_decl->pointed_type_;
    if (newtree == nullptr) return(false);
    InitWithTree(newtree, true, !pointer_decl->isconst_, solver);
    return(true);
}

void ExpressionAttributes::Normalize(ITypedefSolver *solver)
{
    while (exp_type_ == BT_TREE) {
        type_tree_ = SolveTypedefs(type_tree_);
        if (type_tree_ == nullptr) {
            exp_type_ = BT_ERROR;
        } else {
            AstNodeType nodetype = type_tree_->GetType();
            if (nodetype == ANT_BASE_TYPE) {
                exp_type_ = Token2ExpBaseTypes(((AstBaseType*)type_tree_)->base_type_);
            } else {
                // is a complex declaration - can't be reduced to an enum.
                break;
            }
        }
    }
}

// Note: since value computations are independent of type we can split in 2 steps.
bool ExpressionAttributes::UpdateWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error)
{
    if (!UpdateTypeWithBinopOperation(attr_right, operation, error)) {
        value_is_valid_ = false;
        return(false);
    }
    if (!UpdateValueWithBinopOperation(attr_right, operation, error)) {
        return(false);
    }
    is_a_literal_ = is_a_literal_ && attr_right->is_a_literal_;
    return(true);
}

bool ExpressionAttributes::UpdateTypeWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error)
{
    is_a_variable_ = false;         // is a temporary value
    is_writable_ = false;

    if (exp_type_ == BT_ERROR || attr_right->exp_type_ == BT_ERROR) {
        exp_type_ = BT_ERROR;
        return(true);           // silent. An error has been emitted yet.
    }

    ExpBaseTypes right_type = attr_right->exp_type_;

    // string-char sums. The output of a string (literal or not) and any unsigned integer in the 0..0x3fffff range is a string
    // NOTE: at least one of the twos must be a string
    if (operation == TOKEN_PLUS && ((right_type | exp_type_) & BT_STRING) != 0) { 
        if (IsGoodForStringAddition() && attr_right->IsGoodForStringAddition()) {
            exp_type_ = BT_STRING;
            return(true);
        }
    }

    // if not on a string must operate on a number (NOTE: relationals and booleans are processed by other routines).
    if ((exp_type_ & BT_ALL_THE_NUMBERS) == 0 || (right_type & BT_ALL_THE_NUMBERS) == 0) {
        if (operation == TOKEN_PLUS) {
            *error = "Sum operands must be numbers or strings, you can add strings to UNSIGNED ints in the 0..0x3fffff range";
        } else if (operation == TOKEN_AND || operation == TOKEN_OR) {
            *error = "Arithmetic '&' and '|' operands must be integer numbers.";
        } else {
            *error = "Operands must be numbers";
        }
        exp_type_ = BT_ERROR;
        return(false);
    }

    if (!OperationSupportsFloatingPoint(operation) && ((exp_type_ & BT_ALL_INTEGERS) == 0 || (right_type & BT_ALL_INTEGERS) == 0)) {
        *error = "Operation requires integer operands";
        exp_type_ = BT_ERROR;
        return(false);
    }

    // integer promotion
    ExpBaseTypes left_type = IntegerPromote(exp_type_);
    right_type = IntegerPromote(right_type);

    // some operations dont require the operands to be of the same type
    if (operation == TOKEN_POWER || operation == TOKEN_SHL || operation == TOKEN_SHR) {
        if (IsInteger() && attr_right->IsInteger()) {
            exp_type_ = left_type;
            return(true);
        }
    }

    // types match ?
    if (left_type == right_type) {
        exp_type_ = left_type;
        return(true);
    } else if ((left_type & (BT_COMPLEX64 | BT_FLOAT32)) != 0 && (right_type & (BT_COMPLEX64 | BT_FLOAT32)) != 0) {
        exp_type_ = BT_COMPLEX64;
        return(true);
    } else if ((left_type & (BT_COMPLEX128 | BT_FLOAT64)) != 0 && (right_type & (BT_COMPLEX128 | BT_FLOAT64)) != 0) {
        exp_type_ = BT_COMPLEX128;
        return(true);
    //} else if (value_is_valid_ && !attr_right->value_is_valid_) {
    //    if (!IsValueCompatible(right_type)) {
    //        *error = "Left constant value doesn't fit the range of the right operand type.";
    //        exp_type_ = BT_ERROR;
    //    } else {
    //        value_.PerformConversion(ExpBase2TokenTypes(right_type));
    //        exp_type_ = right_type;
    //    }
    //} else if (!value_is_valid_ && attr_right->value_is_valid_) {
    //    if (!attr_right->IsValueCompatible(left_type)) {
    //        *error = "Right constant value doesn't fit the range of the left operand type.";
    //        exp_type_ = BT_ERROR;
    //    } else {
    //        attr_right->value_.PerformConversion(ExpBase2TokenTypes(left_type));
    //        exp_type_ = left_type;
    //    }
    } else {
        *error = "Operand types mismatch.";
        exp_type_ = BT_ERROR;
    }
    return (exp_type_ != BT_ERROR);
}

bool ExpressionAttributes::BinopRequiresNumericConversion(const ExpressionAttributes *attr_left, const ExpressionAttributes *attr_right, Token operation)
{
    ExpBaseTypes left_type = IntegerPromote(attr_left->exp_type_);
    ExpBaseTypes right_type = IntegerPromote(attr_right->exp_type_);

    // some operations dont require the operands to be of the same type
    if (operation == TOKEN_POWER || operation == TOKEN_SHL || operation == TOKEN_SHR) {
        if (attr_left->IsInteger() && attr_right->IsInteger()) {
            return(false);
        }
    }

    // types match ?
    if (left_type == right_type) {
        return(false);
    } else if ((left_type & (BT_COMPLEX64 | BT_FLOAT32)) != 0 && (right_type & (BT_COMPLEX64 | BT_FLOAT32)) != 0) {
        return(false);
    } else if ((left_type & (BT_COMPLEX128 | BT_FLOAT64)) != 0 && (right_type & (BT_COMPLEX128 | BT_FLOAT64)) != 0) {
        return(false);
    }
    return(true);
}

bool ExpressionAttributes::IsGoodForStringAddition(void) const
{
    if (exp_type_ == BT_STRING) return(true);
    if (!IsInteger()) return(false);
    if (!value_is_valid_) return(true);
    return(value_.IsAValidCharValue());
}

bool ExpressionAttributes::UpdateValueWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error)
{
    // only track numeric values (this case includes BT_ERROR)
    if ((exp_type_ & BT_ALL_THE_NUMBERS) == 0 || !value_is_valid_ || !attr_right->value_is_valid_) {
        value_is_valid_ = false;
        is_a_literal_ = false;
        return(true);
    }

    // compute the output value
    switch (value_.PerformTypedOp(&attr_right->value_, operation, ExpBase2TokenTypes(exp_type_))) {
    case NumericValue::OE_ILLEGAL_OP:
        *error = "Operation is not compatible with the operand's types";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_INTEGER_POWER_WRONG:
        *error = "On an integer base you can only use an integer exponent ranging 0 to 63 (but not 0**0)";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_OVERFLOW:
        *error = "Operation causes an overflow";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_INTEGER_OVERFLOW:
        *error = "Operation causes an integer overflow";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_SIGNED_UNSIGNED_MISMATCH:
        *error = "operators signed/unsigned mismatch (can't represent both in 64 bits signed or unsigned notation)";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_SHIFT_TOO_BIG:
        *error = "Shifting so many bits positions has no purpose on a value of such precision (you get 0 or -1)";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_NAN_RESULT:
        *error = "Operation result is undefined (NAN)";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_DIV_BY_0:
        *error = "Divide by 0";
        exp_type_ = BT_ERROR;
        break;
    case NumericValue::OE_NEGATIVE_SHIFT:
        *error = "Shift amount can't be negative";
        exp_type_ = BT_ERROR;
        break;
    default:
        break;
    }
    if (exp_type_ == BT_ERROR) {
        value_is_valid_ = false;
        return(false);
    }
    return (true);
}

// for general operations (binops) types smaller than 32 bits are automatically promoted
bool ExpressionAttributes::IsValueCompatible(ExpBaseTypes the_type)
{
    switch (the_type) {
    case BT_COMPLEX128:
        return(value_.FitsComplex(true));
    case BT_COMPLEX64:
        return(value_.FitsComplex(false));
    case BT_FLOAT64:
        return(value_.FitsFloat(true));
    case BT_FLOAT32:
        return(value_.FitsFloat(false));
    case BT_INT8:
    case BT_INT16:
    case BT_UINT8:
    case BT_UINT16:
    case BT_INT32:
        return(value_.FitsSigned(32));
    case BT_INT64:
        return(value_.FitsSigned(64));
    case BT_UINT32:
        return(value_.FitsUnsigned(32));
    case BT_UINT64:
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
    } else if ((the_type & (BT_INT32 + BT_UINT32)) != 0) {
        *num_bits = 32;
    } else if ((the_type & (BT_INT64 + BT_UINT64)) != 0) {
        *num_bits = 64;
    } else if ((the_type & (BT_FLOAT32 + BT_COMPLEX64)) != 0) {
        *num_bits = 25;     // add 2: one is the implicit leadin 1, the other to compensate the decrement
        *num_exp_bits = 8;
    } else if ((the_type & (BT_FLOAT64 + BT_COMPLEX128)) != 0) {
        *num_bits = 54;     // add 2: one is the implicit leadin 1, the other to compensate the decrement
        *num_exp_bits = 11;
    } else {
        *num_bits = 0;
        *num_exp_bits = 0;
        return;
    }
    if (*has_sign) {
        *num_bits -= 1;
    }
}

bool ExpressionAttributes::UpdateWithRelationalOperation(ITypedefSolver *solver, ExpressionAttributes *attr_right, Token operation, string *error)
{
    bool is_equality_comparison = (operation == TOKEN_EQUAL || operation == TOKEN_DIFFERENT);

    is_a_variable_ = false;         // is a temporary value
    is_writable_ = false;
    value_is_valid_ = false;
    is_a_literal_ = false;

    if (exp_type_ == BT_ERROR || attr_right->exp_type_ == BT_ERROR) {
        exp_type_ = BT_ERROR;
        return(true);           // silent. An error has been emitted yet.
    }
    if ((HasComplexType() || attr_right->HasComplexType())) {
        if (!is_equality_comparison) {
            *error = "Dont' know how to apply an order (i.e. use '>' '<' operators) to complex numbers";
            exp_type_ = BT_ERROR;
            return(false);
        }
    }
    if ((exp_type_ & BT_ALL_THE_NUMBERS) != 0 && (attr_right->exp_type_ & BT_ALL_THE_NUMBERS) != 0) {
        exp_type_ = BT_BOOL;
        return(true);
    }
    if (exp_type_ == BT_STRING && attr_right->exp_type_ == BT_STRING) {
        exp_type_ = BT_BOOL;
        return(true);
    }
    if (exp_type_ == BT_BOOL && attr_right->exp_type_ == BT_BOOL && is_equality_comparison) {
        return(true);
    }
    if (IsEnum() && attr_right->IsEnum()) {
        if (!type_tree_->IsCompatible(attr_right->type_tree_, FOR_EQUALITY)) {
            *error = "You can't compare two enums of different type";
            exp_type_ = BT_ERROR;
            return(false);
        }
        exp_type_ = BT_BOOL;
        return(true);
    }
    if (is_equality_comparison && type_tree_ != nullptr && type_tree_->SupportsEqualOperator() && 
        solver->AreTypeTreesCompatible(type_tree_, attr_right->type_tree_, FOR_EQUALITY) == ITypedefSolver::OK) {
        exp_type_ = BT_BOOL;
        return(true);
    }
    if (is_equality_comparison) {
        *error = "Values are not numbers nor of same type or the type doesn't support the == operator";
    } else {
        *error = "You can only compare two scalar numbers, enums, strings";
    }
    exp_type_ = BT_ERROR;
    return(false);
}

bool ExpressionAttributes::HasComplexType(void) const
{
    return((exp_type_ & BT_ALL_COMPLEX) != 0);
}

bool ExpressionAttributes::UpdateWithBoolOperation(ExpressionAttributes *attr_right, string *error)
{
    is_a_variable_ = false;         // is a temporary value
    is_writable_ = false;
    value_is_valid_ = false;
    is_a_literal_ = false;

    if (exp_type_ == BT_ERROR || attr_right->exp_type_ == BT_ERROR) {
        exp_type_ = BT_ERROR;
        return(true);           // silent. An error has been emitted yet.
    }
    if ((exp_type_ & attr_right->exp_type_ & BT_BOOL) != 0) {
        return(true);
    }
    *error = "You can apply '&&' and '||' only to bools";
    exp_type_ = BT_ERROR;
    return(false);
}

bool ExpressionAttributes::UpdateWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error)
{
    if (!UpdateTypeWithUnaryOperation(operation, solver, error)) {
        return(false);
    }
    if (!UpdateValueWithUnaryOperation(operation, solver, error)) {
        return(false);
    }
    return(true);
}

bool ExpressionAttributes::UpdateTypeWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error)
{
    is_a_variable_ = false;         // except for *, as fixed in ApplyTheIndirectionOperator()
    is_writable_ = false;

    if (operation != TOKEN_MINUS && operation != TOKEN_PLUS && operation != TOKEN_NOT) {
        is_a_literal_ = false;
    }

    if (exp_type_ == BT_ERROR) {
        return(true);
    }
    if (operation == TOKEN_MPY) {
        if (!ApplyTheIndirectionOperator(solver)) {
            *error = "You can use unary * only on pointers and addresses";
            exp_type_ = BT_ERROR;
        }
    } else if (exp_type_ == BT_ADDRESS_OF) {
        *error = "On an address the only allowed operator is '*'";
        exp_type_ = BT_ERROR;
        return(false);
    }
    switch (operation) {
    case TOKEN_SIZEOF:
        assert(false);
        break;                      // do nothing. in case of 'sizeof', attr is reinited with InitWithInt32().
    case TOKEN_DIMOF:
        if (exp_type_ != BT_TREE || type_tree_ == nullptr || (type_tree_->GetType() != ANT_MAP_TYPE && type_tree_->GetType() != ANT_ARRAY_TYPE)) {
            *error = "You can apply the dimof operator only to objects of type 'map' or 'array'";
            exp_type_ = BT_ERROR;
        } else {
            exp_type_ = BT_INT32;
            is_writable_ = false;
        }
        break;
    case TOKEN_MINUS:
        exp_type_ = IntegerPromote(exp_type_);
        if ((exp_type_ & (BT_ALL_THE_NUMBERS - BT_ALL_UINTS)) == 0) {
            *error = "Unary '-' only applies to numeric signed basic types";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_PLUS:
        exp_type_ = IntegerPromote(exp_type_);
        if ((exp_type_ & BT_ALL_THE_NUMBERS) == 0) {
            *error = "Unary '+' only applies to numeric types";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_NOT:
        exp_type_ = IntegerPromote(exp_type_);
        if ((exp_type_ & BT_ALL_INTEGERS) == 0) {
            *error = "The arithmetic not '~' operator requires an integer value";
            exp_type_ = BT_ERROR;
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
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        if ((exp_type_ & (BT_ALL_THE_NUMBERS | BT_STRING)) != 0) {
            exp_type_ = Token2ExpBaseTypes(operation);
        } else {
            *error = "Unallowed conversion.";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_STRING:
        if ((exp_type_ & (BT_ALL_THE_NUMBERS | BT_BOOL)) != 0) {
            exp_type_ = BT_STRING;
        } else {
            *error = "Unallowed conversion.";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_BOOL:
        *error = "Please use relational operators to convert numbers and strings to bools.";
        exp_type_ = BT_ERROR;
        break;
    }
    if (exp_type_ == BT_ERROR) {
        return(false);
    }
    return(true);
}

bool ExpressionAttributes::UpdateValueWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error)
{
    // only track numeric values (this case includes BT_ERROR)
    if ((exp_type_ & BT_ALL_THE_NUMBERS) == 0 || !value_is_valid_) {
        value_is_valid_ = false;
        return(true);
    }

    switch (operation) {
    case TOKEN_MINUS:
        if (value_.PerformTypedUnop(TOKEN_MINUS, ExpBase2TokenTypes(exp_type_)) == NumericValue::OE_INTEGER_OVERFLOW) {
            *error = "Result of sign inversion overflows (sing integer literals range -2**63 to 2**64-1). consider using a float literal";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_NOT:
        value_.PerformConversion(ExpBase2TokenTypes(exp_type_));  // because of integer promotion
        if (value_.PerformTypedUnop(TOKEN_NOT, ExpBase2TokenTypes(exp_type_)) == NumericValue::OE_ILLEGAL_OP) {
            *error = "The arithmetic not '~' operator requires an integer value";
            exp_type_ = BT_ERROR;
        }
        break;
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
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        if (value_.PerformConversion(operation) == NumericValue::OE_OVERFLOW) {
            *error = "Overflow in float to int conversion";
            exp_type_ = BT_ERROR;
        }
        break;
    case TOKEN_PLUS:    
        value_.PerformConversion(ExpBase2TokenTypes(exp_type_));  // because of integer promotion
        break;
    default:
        value_is_valid_ = false;
    }
    if (exp_type_ == BT_ERROR) {
        value_is_valid_ = false;
        return(false);
    }
    return(true);
}

bool ExpressionAttributes::UpdateWithFunCall(vector<ExpressionAttributes> *attr_args, 
                                             vector<AstArgument*> *arg_descs, AstFuncType **func_typedesc, 
                                             ITypedefSolver *solver, string *error)
{
    int     ii;
    char    errorbuf[128];

    value_is_valid_ = false;
    is_a_literal_ = false;

    if (exp_type_ == BT_ERROR) {
        return(true);
    }
    if (exp_type_ != BT_TREE || type_tree_ == nullptr || type_tree_->GetType() != ANT_FUNC_TYPE) {
        *error = "You can apply an argument list only to an object of type 'function'";
        exp_type_ = BT_ERROR;
        return(false);
    }

    // in case of early exit
    exp_type_ = BT_ERROR;

    AstFuncType *typedesc = (AstFuncType*)type_tree_;
    *func_typedesc = typedesc;
    for (ii = 0; ii < (int)arg_descs->size() && ii < (int)typedesc->arguments_.size(); ++ii) {
        AstArgument *argvalue = (*arg_descs)[ii];                   // args from the ast of the expression
        VarDeclaration *argdecl = typedesc->arguments_[ii];         // args from the ast of the declaration
        ExpressionAttributes *argvalue_attr = &(*attr_args)[ii];    // of actual argument expressions
        ExpressionAttributes argdecl_attr;                          // of a fake expression where we pretend the formal argument is a variable.

        if (argvalue == nullptr) {
            if (argdecl->initer_ == nullptr) {
                sprintf(errorbuf, "Argument %d is not specified and has no default value", ii + 1);
                *error = errorbuf;
                return(false);
            }
            continue;
        }

        // if we are here argvalue != nullptr and argvalue_attr is inited.
        // this check should never fail. (if an expression fails yhis function shouldn't be called.
        if (argvalue_attr->exp_type_ == BT_ERROR) {
            sprintf(errorbuf, "Argument %d has a wrong expression", ii + 1);
            *error = errorbuf;
            return(false);
        }

        // check tags
        if (argvalue->name_[0] != 0 && argvalue->name_ != argdecl->name_) {
            sprintf(errorbuf, "Argument %d has mismatching name", ii + 1);
            *error = errorbuf;
            return(false);
        }

        // check types compatibility
        ITypedefSolver::TypeMatchResult typematch = ITypedefSolver::OK;
        argdecl_attr.InitWithTree(argdecl->type_spec_, true, true, solver);
        if (argdecl->HasOneOfFlags(VF_READONLY)) {
            // NOTE: is read only inside the function, but must be written by the caller !!!
            if (GetParameterPassingMethod(argdecl->type_spec_, true) == PPM_VALUE) {
                if (!argdecl_attr.CanAssign(argvalue_attr, solver, error)) {  

                    // note: error message provided by CanAssign                 
                    return(false);
                }
            } else {
                typematch = solver->AreTypeTreesCompatible(argdecl->type_spec_, argvalue_attr->type_tree_, FOR_REFERENCING);
            }
        } else { 

            // out or io : is actual parameter writable ?
            if (!argvalue_attr->is_a_variable_ || !argvalue_attr->is_writable_) {
                sprintf(errorbuf, "Argument %d is an output argument and needs to be a writable variable", ii + 1);
                *error = errorbuf;
                return(false);
            }

            // type check !!
            if (argvalue_attr->exp_type_ == argdecl_attr.exp_type_) {
                if (argvalue_attr->exp_type_ == BT_TREE) {
                    typematch = solver->AreTypeTreesCompatible(argdecl->type_spec_, argvalue_attr->type_tree_, FOR_REFERENCING);
                } else {
                    typematch = ITypedefSolver::OK;
                }
            }
        }
        if (typematch == ITypedefSolver::KO) {
            sprintf(errorbuf, "Argument %d type mismatch", ii + 1);
            *error = errorbuf;
            return(false);
        } else if (typematch == ITypedefSolver::CONST) {
            sprintf(errorbuf, "Argument %d pointer constness mismatch", ii + 1);
            *error = errorbuf;
            return(false);
        }
    }
    if (ii != (int)arg_descs->size() && !typedesc->varargs_) {
        *error = "Too many arguments";
        return(false);
    }
    for (; ii < (int)typedesc->arguments_.size(); ++ii) {
        if (typedesc->arguments_[ii]->initer_ == nullptr) {
            sprintf(errorbuf, "Argument %d is not specified and has no default value", ii + 1);
            *error = errorbuf;
            return(false);
        }
    }
    InitWithTree(typedesc->return_type_, false, false, solver);
    return(true);
}

AstFuncType *ExpressionAttributes::GetFunCallType(void) const
{
    if (exp_type_ == BT_ERROR || exp_type_ != BT_TREE || type_tree_ == nullptr || type_tree_->GetType() != ANT_FUNC_TYPE) {
        return(nullptr);
    }
    return((AstFuncType*)type_tree_);
}

bool ExpressionAttributes::UpdateWithIndexing(ExpressionAttributes *low_attr, ExpressionAttributes *high_attr, AstIndexing *node, 
                                              AstMapType **map_typedesc, ITypedefSolver *solver, string *error)
{
    value_is_valid_ = false;
    is_a_literal_ = false;

    *map_typedesc = nullptr;
    if (exp_type_ == BT_ERROR) {
        return(true);
    }
    if (exp_type_ != BT_TREE || type_tree_ == nullptr || (type_tree_->GetType() != ANT_MAP_TYPE && type_tree_->GetType() != ANT_ARRAY_TYPE)) {
        *error = "You can apply an index list only to objects of type 'map' or 'array' (did you specify too many indices ?)";
        exp_type_ = BT_ERROR;
        return(false);
    }
    exp_type_ = BT_ERROR;
    if (type_tree_->GetType() == ANT_MAP_TYPE) {
        AstMapType *typedesc = (AstMapType*)type_tree_;
        *map_typedesc = typedesc;
        ExpressionAttributes argdecl_attr;                          // of a fake expression where we pretend the map input is a variable.

        if (!node->is_single_index_) {
            *error = "Maps accept a single index (not ranges)";
            return(false);
        }
        argdecl_attr.InitWithTree(typedesc->key_type_, true, true, solver);
        if (argdecl_attr.CanAssign(low_attr, solver, error)) {
            InitWithTree(typedesc->returned_type_, true, is_writable_, solver);
            return(true);
        }
    } else {
        AstArrayType *typedesc = (AstArrayType*)type_tree_;

        // let's make typedesc->dimensions_ valid
        assert(typedesc->dimension_was_computed_);
        //solver->CheckArrayIndicesInTypes(typedesc);

        // if dimension_ is defined (>0) the index must fall in the range.
        size_t  index_value;
        if (!low_attr->IsOnError()) {   // uninited descriptors are marked as on error !
            if (!low_attr->IsAValidArrayIndex(&index_value)) {
                *error = "Indices must be positive integers";
                return(false);
            } else {
                size_t array_size = typedesc->dimension_;
                if (array_size > 0 && index_value >= array_size) {
                    *error = "Index value is out of bound";
                    return(false);
                }
            }
        }
        if (!high_attr->IsOnError()) {   // uninited descriptors are marked as on error !
            if (!high_attr->IsAValidArrayIndex(&index_value)) {
                *error = "Indices must be positive integers";
                return(false);
            } else {
                size_t array_size = typedesc->dimension_;
                if (array_size > 0 && index_value >= array_size) {
                    *error = "Index value is out of bound";
                    return(false);
                }
            }
        }
        InitWithTree(typedesc->element_type_, true, is_writable_, solver);
        return(true);
    }
    return(false);
}

bool ExpressionAttributes::TakeAddress(void)
{
    value_is_valid_ = false;
    is_a_literal_ = false;
    if (exp_type_ == BT_ERROR) return(true);
    if (!is_a_variable_ || (exp_type_ & (BT_ALL_THE_NUMBERS | BT_STRING | BT_BOOL)) == 0 && exp_type_ != BT_TREE) {
        return(false);
    }
    exp_pointed_type_ = exp_type_;
    exp_type_ = BT_ADDRESS_OF;
    is_a_variable_ = false;
    return(true);
}

IAstTypeNode *ExpressionAttributes::GetIteratorType(void) const
{
    if (exp_type_ != BT_TREE || type_tree_ == nullptr || !is_a_variable_) return(nullptr);
    switch (type_tree_->GetType()) {
    case ANT_ARRAY_TYPE:
        return(((AstArrayType*)type_tree_)->element_type_);
    case ANT_MAP_TYPE:
        return(((AstMapType*)type_tree_)->returned_type_);
    default:
        break;
    }
    return(nullptr); 
}

bool ExpressionAttributes::GetIntegerIteratorType(Token *ittype, ExpressionAttributes* low, ExpressionAttributes *high, ExpressionAttributes *step)
{
    if (low->FitsSigned(32) && high->FitsSigned(32) && step->FitsSigned(32)) {
        *ittype = TOKEN_INT32;
        return(true);
    }
    if (low->FitsSigned(64) && high->FitsSigned(64) && step->FitsSigned(64)) {
        *ittype = TOKEN_INT64;
        return(true);
    }
    return(false);
}

bool ExpressionAttributes::FitsUnsigned(int nbits) const
{
    if ((exp_type_ & BT_ERROR) != 0) return(true);  // just let's pretend to limit the number of error messages
    if (value_is_valid_) {
        return(value_.FitsUnsigned(nbits));
    }
    if (nbits >= 8 && exp_type_ == BT_UINT8) {
        return(true);
    }
    if (nbits >= 16 && exp_type_ == BT_UINT16) {
        return(true);
    }
    if (nbits >= 32 && exp_type_ == BT_UINT32) {
        return(true);
    }
    if (nbits >= 64 && exp_type_ == BT_UINT64) {
        return(true);
    }
    return(false);
}

bool ExpressionAttributes::FitsSigned(int nbits) const
{
    if ((exp_type_ & BT_ERROR) != 0) return(true);  // just let's pretend to limit the number of error messages
    if (value_is_valid_) {
        return(value_.FitsSigned(nbits));
    }
    if (nbits >= 8 && (exp_type_ & BT_INT8) != 0) {
        return(true);
    }
    if (nbits >= 16 && (exp_type_ & (BT_INT16 | BT_UINT8)) != 0) {
        return(true);
    }
    if (nbits >= 32 && (exp_type_ & (BT_INT32 | BT_UINT16)) != 0) {
        return(true);
    }
    if (nbits >= 64 && (exp_type_ & (BT_INT64 | BT_UINT32)) != 0) {
        return(true);
    }
    return(false);
}

bool ExpressionAttributes::CanAssign(ExpressionAttributes *src, ITypedefSolver *solver, string *error) const
{
    if (exp_type_ == BT_ERROR || src->exp_type_ == BT_ERROR) {
        return(true); // be silent
    }
    if (!is_a_variable_) {
        *error = "You can assign only to a writable variable or a portion of a variable";
        return(false);
    }
    if (!is_writable_) {
        *error = "Writing to a read-only variable";
        return(false);
    }
    bool can_assign = false;
    if ((exp_type_ & BT_ALL_THE_NUMBERS) != 0) {
        if ((src->exp_type_ & BT_ALL_THE_NUMBERS) != 0) {
            if (src->value_is_valid_) {
                can_assign = src->IsValueCompatibleForAssignment(exp_type_);
                if (!can_assign) {
                    *error = "Value doesn't fit the destination (assignment would cause loss of range or precision).";
                }
            } else {
                int32_t num_bits1, num_bits2, num_exp_bits1, num_exp_bits2;
                bool    has_sign1, has_sign2, is_complex1, is_complex2;

                GetBaseTypePrecision(     exp_type_, &num_bits1, &num_exp_bits1, &has_sign1, &is_complex1);
                GetBaseTypePrecision(src->exp_type_, &num_bits2, &num_exp_bits2, &has_sign2, &is_complex2);
                can_assign = (is_complex1 || !is_complex2) && (has_sign1 || !has_sign2) &&
                             num_bits1 >= num_bits2 && num_exp_bits1 >= num_exp_bits2;
                if (!can_assign) {
                    *error = "Type mismatch in assignment. Please cast explicitly";
                }
            }
        } else {
            can_assign = false;
        }
    } else if (exp_type_ == BT_STRING) {
        if (!src->IsGoodForStringAddition()) {
            *error = "You can assign to a string another string or an unsigned integer max 32 bits in the 0..0x3fffff range";
            return(false);
        }
        can_assign = true;
    } else if (exp_type_ == BT_BOOL) {
        can_assign = src->exp_type_ == BT_BOOL;
        if (!can_assign) {
            *error = "You can assign a bool only with a bool (use relational operators to generate bools)";
            return(false);
        }
    } else if (exp_type_ == BT_TREE) {
        // assignment of a pointer with an address or with another pointer
        if (src->exp_type_ == BT_ADDRESS_OF || src->exp_type_ == BT_LITERAL_NULL) {
            IAstTypeNode *dst_tree = SolveTypedefs(type_tree_);
            if (dst_tree == nullptr) return(true);  // silent
            if (dst_tree->GetType() == ANT_POINTER_TYPE) {
                if (src->exp_type_ == BT_LITERAL_NULL) {
                    can_assign = true;
                } else {
                    AstPointerType *ptr_desc = (AstPointerType*)dst_tree;
                    switch (solver->AreTypeTreesCompatible(ptr_desc->pointed_type_, src->type_tree_, FOR_REFERENCING)) {
                    case ITypedefSolver::OK:
                        can_assign = true;
                        break;
                    case ITypedefSolver::KO:
                        can_assign = false;
                        break;
                    case ITypedefSolver::CONST:
                        can_assign = false;
                        *error = "Pointer constness mismatch";
                        break;
                    }
                    if (can_assign && !src->is_writable_ && !ptr_desc->isconst_) {
                        can_assign = false;
                        *error = "Need a const pointer here";
                    }
                }
            } else {
                can_assign = false;
            }
        } else if (src->exp_type_ == BT_TREE) {
            switch (solver->AreTypeTreesCompatible(type_tree_, src->type_tree_, FOR_ASSIGNMENT)) {
            case ITypedefSolver::OK:
                can_assign = true;
                break;
            case ITypedefSolver::KO:
                can_assign = false;
                break;
            case ITypedefSolver::CONST:
                can_assign = false;
                *error = "Pointer constness mismatch";
                break;
            case ITypedefSolver::NONCOPY:
                can_assign = false;
                *error = "Classes with finalize or with a non-copyable member are not copyable";
                break;
            }
        } else {
            can_assign = false;
        }
    }
    if (!can_assign && *error == "") {
        *error = "Type mismatch/No known conversion";
    }
    return(can_assign);
}

bool ExpressionAttributes::HasIntegerType(void) const
{ 
    return((exp_type_ & BT_ALL_INTEGERS) != 0);
}

bool ExpressionAttributes::IsValueCompatibleForAssignment(ExpBaseTypes the_type)
{
    switch (the_type) {
    case BT_COMPLEX128:
        return(value_.FitsComplex(true));
    case BT_COMPLEX64:
        return(value_.FitsComplex(false));
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
        return(value_.FitsUnsigned(32));
    case BT_UINT64:
        return(value_.FitsUnsigned(64));
    default:
        break;
    }
    return(false);
}

ExpBaseTypes ExpressionAttributes::IntegerPromote(ExpBaseTypes the_type)
{
    switch (the_type) {
    case BT_INT8:
    case BT_INT16:
    case BT_UINT8:
    case BT_UINT16:
        return(BT_INT32);
    default:
        break;
    }
    return(the_type);
}

bool ExpressionAttributes::RequiresPromotion(void) const
{
    switch (exp_type_) {
    case BT_INT8:
    case BT_INT16:
    case BT_UINT8:
    case BT_UINT16:
        return(true);
    default:
        break;
    }
    return(false);
}


// returns 0 if not known at run time
bool ExpressionAttributes::IsAValidArrayIndex(size_t *value) const
{
    *value = 0;
    if ((exp_type_ & BT_ALL_INTEGERS) == 0 && !IsEnum()) {
        return(false);
    }
    if (value_is_valid_) {
        if (!value_.IsAPositiveInteger()) {
            return(false);
        }
        *value = (size_t)value_.GetUnsignedIntegerValue();
    }
    return(true);
}

// returns 0 if not known at run time
bool ExpressionAttributes::IsAValidArraySize(size_t *value) const
{
    *value = 0;
    if ((exp_type_ & BT_ALL_INTEGERS) == 0 && !IsEnum()) {
        return(false);
    }
    if (!value_is_valid_) {
        return(false);
    }
    if (!value_.IsAPositiveInteger()) {
        return(false);
    }
    *value = (size_t)value_.GetUnsignedIntegerValue();
    return(true);
}

AstArrayType *ExpressionAttributes::GetVectorType(void) const
{
    if (exp_type_ != BT_TREE || type_tree_ == nullptr || !is_a_variable_) return(nullptr);
    if (type_tree_->GetType() != ANT_ARRAY_TYPE) return(nullptr);
    return((AstArrayType*)type_tree_);
}

bool ExpressionAttributes::GetSignedIntegerValue(int64_t *value) const
{
    if (!value_is_valid_) return(false);
    *value = value_.GetSignedIntegerValue();
    return(true);
}

IAstTypeNode *ExpressionAttributes::GetAutoType(void) const
{
    // NOTE: if is_a_variable_ it was inited with InitWithTree()
    if (exp_type_ == BT_TREE || is_a_variable_) return(original_tree_);
    return(nullptr);
}

Token ExpressionAttributes::GetAutoBaseType(void) const
{
    Token val = ExpBase2TokenTypes(exp_type_);
    if (val == TOKEN_VOID) val = TOKENS_COUNT;
    return(val);
}

IAstTypeNode *ExpressionAttributes::GetPointedType(void) const
{
    // NOTE: if we took the address, it is _a_variable_ and was inited with InitWithTree()
    if (exp_type_ == BT_TREE && type_tree_->GetType() == ANT_POINTER_TYPE) {
        AstPointerType *ptr = (AstPointerType*)type_tree_;
        return(ptr->pointed_type_);
    }
    return(nullptr);
}

IAstTypeNode *ExpressionAttributes::GetTypeFromAddressOperator(bool *writable) const
{
    // NOTE: if we took the address, it is _a_variable_ and was inited with InitWithTree()
    if (exp_type_ == BT_ADDRESS_OF) {
        *writable = is_writable_;
        return(original_tree_);
    }
    return(nullptr);
}

bool ExpressionAttributes::CanAssignWithoutLoss(Token dst, Token src)
{
    int32_t dst_bits, src_bits, dst_exp, src_exp;
    bool    dst_complex, src_complex, dst_signed, src_signed;

    GetBaseTypePrecision(Token2ExpBaseTypes(dst), &dst_bits, &dst_exp, &dst_signed, &dst_complex);
    GetBaseTypePrecision(Token2ExpBaseTypes(src), &src_bits, &src_exp, &src_signed, &src_complex);
    return(dst_bits >= src_bits && dst_exp >= src_exp && (dst_signed || !src_signed) && (dst_complex || !src_complex));
}

bool ExpressionAttributes::IsEnum(void) const
{
    return(exp_type_ == BT_TREE && type_tree_ != nullptr && type_tree_->GetType() == ANT_ENUM_TYPE);
}

bool ExpressionAttributes::IsCaseValueCompatibleWithSwitchExpression(ExpressionAttributes *switch_expression)
{
    return(IsValueCompatible(switch_expression->exp_type_));
}

} // namespace