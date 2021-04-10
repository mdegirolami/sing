#ifndef EXPRESSION_ATTRIBUTES_H
#define EXPRESSION_ATTRIBUTES_H

#include "numeric_value.h"
//#include "ast_nodes.h"
//#include "ast_checks.h"

namespace SingNames {

class AstArrayType;
class IAstTypeNode;
class ITypedefSolver;
class AstArgument;
class AstFuncType;
class AstIndexing;
class AstMapType;
class VarDeclaration;

enum class ExpressionUsage {WRITE, READ, NONE, BOTH};

enum ExpBaseTypes {

    // numerics
    BT_INT8 = 1,
    BT_INT16 = 2,
    BT_INT32 = 4,
    BT_INT64 = 8,
    BT_UINT8 = 0x10,
    BT_UINT16 = 0x20,
    BT_UINT32 = 0x40,
    BT_UINT64 = 0x80,
    BT_FLOAT32 = 0x100,
    BT_FLOAT64 = 0x200,
    BT_COMPLEX64 = 0x400,
    BT_COMPLEX128 = 0x800,


    // others
    BT_STRING = 0x4000,
    BT_BOOL = 0x8000,

    // special values
    BT_VOID = 0x10000,
    BT_LITERAL_NULL = 0x80000,
    BT_TREE = 0,          // means that you need to examine type_tree_
    BT_ERROR = 0x100000,    // problems in a referenced type declaration. ignore further errors
    BT_ADDRESS_OF = 0x200000,    // see exp_pointed_type_

    // MASKS INCLUDING ONE OR MORE OF THE PREVIOUS ONES
    BT_ALL_UINTS = 0xf0,
    BT_ALL_INTS = 0x0f,
    BT_ALL_INTEGERS = 0xff,
    BT_ALL_FLOATS = 0x0300,
    BT_ALL_COMPLEX = 0x0c00,
    BT_ALL_THE_NUMBERS = 0xfff,
};

ExpBaseTypes Token2ExpBaseTypes(Token token);
Token ExpBase2TokenTypes(ExpBaseTypes basetype);

class ExpressionAttributes {
public:
    ExpressionAttributes();

    // use these to init and update the attribute descriptor.
    void InitWithTree(IAstTypeNode *tree, VarDeclaration *var, bool is_a_variable, bool is_writable, ITypedefSolver *solver);
    void SetTheValueFrom(const ExpressionAttributes *attr);
    void SetEnumValue(int32_t value);
    void InitWithInt32(int value);
    bool InitWithLiteral(Token literal_type, const char *value, const char *img_value, bool isint, bool realneg, bool imgneg, string *error);
    bool UpdateWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error); // returns false if fails
    bool UpdateWithRelationalOperation(ITypedefSolver *solver, ExpressionAttributes *attr_right, Token operation, string *error); // returns false if fails
    bool UpdateWithBoolOperation(ExpressionAttributes *attr_right, string *error); // returns false if fails
    bool UpdateWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error); // returns false if fails
    bool UpdateWithFunCall(vector<ExpressionAttributes> *attr_args, vector<AstArgument*> *arg_descs, 
                           AstFuncType **func_typedesc, ITypedefSolver *solver, string *error, int *wrong_arg_idx);
    AstFuncType *GetFunCallType(void) const; // returns nullptr if the type is not a funcall
    bool UpdateWithIndexing(ExpressionAttributes *low_attr, ExpressionAttributes *high_attr, AstIndexing *node, 
                            ITypedefSolver *solver, string *error);
    void SetError(void) { exp_type_ = BT_ERROR; value_is_valid_ = false; }
    bool TakeAddress(void);
    void SetUsageFlags(ExpressionUsage usage);

    // Queries. Don't affect the object
    bool IsOnError(void) const { return(exp_type_ == BT_ERROR); }
    IAstTypeNode *GetIteratorType(void) const;
    static bool GetIntegerIteratorType(Token *ittype, ExpressionAttributes* low, ExpressionAttributes *high, ExpressionAttributes *step);
    bool GetSignedIntegerValue(int64_t *value) const;
    bool CanAssign(ExpressionAttributes *src, ITypedefSolver *solver, string *error) const;
    bool CanSwap(ExpressionAttributes *src, ITypedefSolver *solver) const;
    bool IsVoid(void)  const { return(exp_type_ == BT_VOID); }
    bool IsBool(void)  const { return(exp_type_ == BT_BOOL); }
    bool IsString(void) const { return(exp_type_ == BT_STRING); }
    bool IsInteger(void) const { return((exp_type_ & BT_ALL_INTEGERS) != 0); }
    bool IsFloat(void) const { return((exp_type_ & BT_ALL_FLOATS) != 0); }
    bool IsComplex(void) const { return((exp_type_ & BT_ALL_COMPLEX) != 0); }
    bool IsScalar(void) const { return((exp_type_ & (BT_ALL_INTEGERS + BT_ALL_FLOATS)) != 0); }
    bool IsUnsignedInteger(void) const { return((exp_type_ & BT_ALL_UINTS) != 0); }
    bool IsNumber(void) const { return((exp_type_ & BT_ALL_THE_NUMBERS) != 0); }
    bool IsLiteralNull(void) const { return(exp_type_ == BT_LITERAL_NULL); }
    bool HasIntegerType(void) const;
    bool HasComplexType(void) const;
    bool CanIncrement(void)  const { return((exp_type_ & BT_ALL_INTEGERS) != 0 && is_a_variable_ && is_writable_); }
    bool IsAValidArrayIndex(size_t *value) const;
    bool IsAValidArraySize(size_t *value) const;
    AstArrayType *GetVectorType(void) const;  // returns NULL if not a vector
    IAstTypeNode *GetAutoType(void) const;
    Token GetAutoBaseType(void) const;
    IAstTypeNode *GetTypeTree(void) const { return(type_tree_); };
    IAstTypeNode *GetPointedType(void) const;
    IAstTypeNode *GetTypeFromAddressOperator(bool *writable) const;
    bool HasKnownValue(void) const { return(value_is_valid_); }
    uint64_t GetUnsignedValue(void) const { return(value_.GetUnsignedIntegerValue()); }
    double GetDoubleValue(void) const { return(value_.GetDouble()); }
    const NumericValue *GetValue(void) const { return(&value_); }
    bool FitsUnsigned(int nbits) const;
    bool FitsSigned(int nbits) const;
    bool IsGoodForStringAddition(void) const;
    bool RequiresPromotion(void) const;
    static bool BinopRequiresNumericConversion(const ExpressionAttributes *attr_left, const ExpressionAttributes *attr_right, Token operation);
    static bool CanAssignWithoutLoss(Token dst, Token src);
    bool IsAVariable(void) const { return(is_a_variable_); }
    bool IsEnum(void) const;
    bool IsArray(void) const;
    bool IsMap(void) const;
    bool IsFunc(void) const;
    bool IsPointer(void) const;
    bool IsWeakPointer(void) const;
    bool IsStrongPointer(void) const;
    bool IsWritable(void) { return(is_writable_); }
    bool IsCaseValueCompatibleWithSwitchExpression(ExpressionAttributes *switch_expression);

private:
    ExpBaseTypes    exp_type_;
    ExpBaseTypes    exp_pointed_type_;      // in case exp_type_ == BT_ADDRESS_OF
    IAstTypeNode    *type_tree_;            // in case exp_type_ == BT_CT_TREE
    IAstTypeNode    *original_tree_;        // before typedef solution. Used solely for auto typing.
    bool            is_writable_;           // is valid if is_a_variable_. in case exp_type_ == BT_ADDRESS_OF keeps info about the pointed var.
    bool            is_a_literal_;          // a literal expression can be cast to a lesser type if range and precision are preserved

    bool            is_a_variable_;         // the expression is backed by a variable.
    VarDeclaration  *variable_;             // backing var.

    NumericValue    value_;                 // numeric value of the expression
    bool            value_is_valid_;        // always true for BT_LITERAL_NUMBER, often true for numeric values

    bool UpdateTypeWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error);
    bool UpdateValueWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error);
    bool UpdateTypeWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error); // returns false if fails
    bool UpdateValueWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error); // returns false if fails
    void Normalize(ITypedefSolver *solver);                     // if the pointed tree is a basic type, move it to attr->exp_type_
    bool ApplyTheIndirectionOperator(ITypedefSolver *solver);   // returns false if fails
    bool OperationSupportsFloatingPoint(Token operation);
    bool OperationSupportsComplex(Token operation);
    ExpBaseTypes CanPromote(ExpBaseTypes op1, ExpBaseTypes op2);
    static void GetBaseTypePrecision(ExpBaseTypes the_type, int32_t *num_bits, int32_t *num_exp_bits, bool *has_sign, bool *is_complex);
    static ExpBaseTypes IntegerPromote(ExpBaseTypes the_type);

    // deso the value fit a given type ?
    bool IsValueCompatible(ExpBaseTypes the_type);
    bool IsValueCompatibleForAssignment(ExpBaseTypes the_type);
};

} // namespace

#endif
