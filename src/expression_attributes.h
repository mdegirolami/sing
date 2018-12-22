#ifndef EXPRESSION_ATTRIBUTES_H
#define EXPRESSION_ATTRIBUTES_H

#include "numeric_value.h"
#include "ast_nodes.h"
//#include "ast_checks.h"

namespace SingNames {

enum ExpBaseTypes {
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
    BT_SIZE_T = 0x1000,
    BT_RUNE = 0x2000,
    BT_STRING = 0x4000,
    BT_BOOL = 0x8000,
    BT_VOID = 0x10000,

    // literals
    BT_LITERAL_NUMBER = 0x20000,    // value_ is valid
    BT_LITERAL_STRING = 0x40000,
    BT_LITERAL_NULL = 0x80000,
    BT_TREE = 0,          // means that you need to examine type_tree_
    BT_ERROR = 0x100000,    // problems in a referenced type declaration. ignore further errors
    BT_ADDRESS_OF = 0x200000,    // see exp_pointed_type_

    // MASKS INCLUDING ONE OR MORE OF THE PREVIOUS ONES
    BT_ALL_UINTS = 0x30f0,
    BT_ALL_INTS = 0x000f,
    BT_ALL_FLOATS = 0x0300,
    BT_ALL_COMPLEX = 0x0c00,
    BT_ALL_THE_NUMBERS = 0x3fff,

    BT_NUMBERS_AND_LITERALS = 0x23fff,      // numbers and literal numbers
    BT_STRINGS_AND_LITERALS = 0x44000       // strings and literal strings
};

ExpBaseTypes Token2ExpBaseTypes(Token token);

// solves a type <name> <def> declaration
class ITypedefSolver {
public:
    virtual IAstTypeNode *TypeFromTypeName(const char *name) = 0;
    virtual bool CheckArrayIndices(AstArrayOrMatrixType *array) = 0;
};

class ExpressionAttributes {
public:
    ExpressionAttributes() : type_tree_(NULL), is_a_left_value_(false), exp_type_(BT_ERROR) {}

    void InitWithTree(IAstTypeNode *tree, bool is_a_variable, ITypedefSolver *solver);
    void InitWithLiteral(Token literal_type, const char *value);
    void Normalize(ITypedefSolver *solver);                     // if the pointed tree is a basic type, move it to attr->exp_type_
    bool UpdateWithBinopOperation(ExpressionAttributes *attr_right, Token operation, string *error); // returns false if fails
    bool UpdateWithRelationalOperation(ExpressionAttributes *attr_right, Token operation, string *error); // returns false if fails
    bool UpdateWithBoolOperation(ExpressionAttributes *attr_right, string *error); // returns false if fails
    bool UpdateWithUnaryOperation(Token operation, ITypedefSolver *solver, string *error); // returns false if fails
    bool UpdateWithFunCall(vector<ExpressionAttributes> *attr_args, vector<AstArgument*> *arg_descs, ITypedefSolver *solver, string *error);
    bool UpdateWithIndexing(vector<ExpressionAttributes> *attr_args, AstIndexing *node, ITypedefSolver *solver, string *error);
    bool IsOnError(void) { return(exp_type_ == BT_ERROR); }
    bool SetError(void) { return(exp_type_ = BT_ERROR); }
    bool TakeAddress(void);
    bool CanAssign(ExpressionAttributes *src, ITypedefSolver *solver, string *error);
    IAstTypeNode *GetTypeTree() { return(exp_type_ == BT_TREE ? type_tree_ : NULL); }
    bool IsAnAddress(void) { return((exp_type_ & (BT_LITERAL_NULL | BT_ADDRESS_OF)) != 0); }
    bool IsAValidArrayIndex(size_t *value);
    bool IsAValidArraySize(size_t *value); // value is 0 if not found at compile time.
    bool IsVoid(void) { return(exp_type_ == BT_VOID); }
    bool IsBool(void) { return(exp_type_ == BT_BOOL); }
    bool CanIncrement(void) { return((exp_type_ & (BT_ALL_UINTS | BT_ALL_INTS)) != 0 && is_a_left_value_); }
    bool IsGoodForAuto(void) { return((exp_type_ & (BT_VOID | BT_LITERAL_NUMBER | BT_LITERAL_NULL | BT_ERROR)) == 0); }

private:
    ExpBaseTypes    exp_type_;
    ExpBaseTypes    exp_pointed_type_;      // in case exp_type_ == BT_ADDRESS_OF
    IAstTypeNode    *type_tree_;            // in case exp_type_ == BT_CT_TREE
    NumericValue    value_;
    bool            is_a_left_value_;
    bool            is_a_variable;          // the expression is backed by 

    bool ApplyTheIndirectionOperator(ITypedefSolver *solver);   // returns false if fails
    bool IsComplex(void);
    bool IsValueCompatible(ExpBaseTypes the_type);
    bool OperationSupportsFloatingPoint(Token operation);
    ExpBaseTypes CanPromote(ExpBaseTypes op1, ExpBaseTypes op2);
    void GetBaseTypePrecision(ExpBaseTypes the_type, int32_t *num_bits, int32_t *num_exp_bits, bool *has_sign, bool *is_complex);
    bool IsValueCompatibleForAssignment(ExpBaseTypes the_type);
    bool            AreTypeTreesCompatible(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode, ITypedefSolver *solver);
    IAstTypeNode    *SolveTypedefs(IAstTypeNode *begin, ITypedefSolver *solver);
};

} // namespace

#endif
