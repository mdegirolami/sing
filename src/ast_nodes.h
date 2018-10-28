#ifndef AST_NODES_H
#define AST_NODES_H

#include "string"
#include "lexer.h"
#include "NamesList.h"

//
// Adding a new node
//
// Declare the node as inheriting from IAstNode
// add the appropriate functions to IAstVisitor
// add a new value to NodeType enum
// make a RAII constructor
// make a destructor which deletes the childrens
// implement IAstNode functions
// add other construction routines as needed 
// 
namespace SingNames {

class IAstVisitor {
public:
    // base structure
    virtual void File(const char *package) = 0;
    virtual void PackageRef(const char *path, const char *package_name) = 0;
    virtual void BeginVarDeclaration(const char *name, bool isvolatile, bool has_initer) = 0;
    virtual void EndVarDeclaration(const char *name, bool isvolatile, bool has_initer) = 0;
    virtual void BeginConstDeclaration(const char *name) = 0;
    virtual void EndConstDeclaration(const char *name) = 0;
    virtual void BeginTypeDeclaration(const char *name) = 0;
    virtual void EndTypeDeclaration(const char *name) = 0;
    virtual void BeginFuncDeclaration(const char *name, bool ismember, const char *classname) = 0;
    virtual void EndFuncDeclaration(const char *name, bool ismember, const char *classname) = 0;
    virtual void BeginIniter(void) = 0;
    virtual void EndIniter(void) = 0;

    // statements
    virtual void BeginBlock(void) = 0;
    virtual void EndBlock(void) = 0;
    virtual void BeginAssignments(int num_assegnee) = 0;
    virtual void EndAssignments(int num_assegnee) = 0;
    virtual void BeginUpdateStatement(Token type) = 0;
    virtual void EndUpdateStatement(Token type) = 0;
    virtual void BeginLeftTerm(int index) = 0;
    virtual void BeginRightTerm(int index) = 0;
    virtual void BeginIncDec(Token type) = 0;
    virtual void EndIncDec(Token type) = 0;
    virtual void BeginWhile(void) = 0;
    virtual void EndWhile(void) = 0;
    virtual void BeginIf(void) = 0;
    virtual void EndIf(void) = 0;
    virtual void BeginIfClause(int num) = 0;
    virtual void EndIfClause(int num) = 0;
    virtual void BeginFor(const char *index, const char *iterator) = 0;
    virtual void EndFor(const char *index, const char *iterator) = 0;
    virtual void BeginForSet(void) = 0;
    virtual void EndForSet(void) = 0;
    virtual void BeginForLow(void) = 0;
    virtual void EndForLow(void) = 0;
    virtual void BeginForHigh(void) = 0;
    virtual void EndForHigh(void) = 0;
    virtual void BeginForStep(void) = 0;
    virtual void EndForStep(void) = 0;
    virtual void SimpleStatement(Token token) = 0;
    virtual void BeginReturn(void) = 0;
    virtual void EndReturn(void) = 0;

    // expressions
    virtual void ExpLeaf(Token type, const char *value) = 0;
    virtual void BeginUnop(Token subtype) = 0;
    virtual void EndUnop(Token subtype) = 0;
    virtual void BeginBinop(Token subtype) = 0;
    virtual void BeginBinopSecondArg(void) = 0;
    virtual void EndBinop(Token subtype) = 0;
    virtual void BeginFunCall(void) = 0;
    virtual void EndFunCall(void) = 0;
    virtual void FunCallArg(int num) = 0;
    virtual void BeginArgument(const char *name) = 0;
    virtual void EndArgument(const char *name) = 0;
    virtual void CastTypeBegin(void) = 0;
    virtual void CastTypeEnd(void) = 0;
    virtual void BeginIndexing(void) = 0;
    virtual void EndIndexing(void) = 0;
    virtual void Index(int num, bool has_lower_bound, bool has_upper_bound) = 0;

    // types
    virtual void BeginFuncType(bool ispure_, bool varargs_, int num_args) = 0;
    virtual void EndFuncType(bool ispure_, bool varargs_, int num_args) = 0;
    virtual void BeginArgumentDecl(Token direction, const char *name, bool has_initer) = 0;
    virtual void EndArgumentDecl(Token direction, const char *name, bool has_initer) = 0;
    virtual void BeginArrayOrMatrixType(bool is_matrix_, int dimensions_count) = 0;
    virtual void EndArrayOrMatrixType(bool is_matrix_, int dimensions_count) = 0;
    virtual void ConstIntExpressionValue(int value) = 0;
    virtual void BeginMapType(void) = 0;
    virtual void MapReturnType(void) = 0;
    virtual void EndMapType(void) = 0;
    virtual void BeginPointerType(bool isconst ,bool isweak) = 0;
    virtual void EndPointerType(bool isconst, bool isweak) = 0;
    virtual void NameOfType(const char *name, int component_index) = 0; // package may be NULL
    virtual void BaseType(Token token) = 0;
};

enum AstNodeType {
    ANT_FILE, ANT_DEPENDENCY, ANT_VAR, ANT_CONST, ANT_TYPE, ANT_INITER, ANT_FUNC,
    ANT_BASE_TYPE, ANT_NAMED_TYPE, ANT_QUALIFIED_TYPE, ANT_ARRAY_TYPE, ANT_MAP_TYPE, ANT_POINTER_TYPE, ANT_FUNC_TYPE,
    ANT_ARGUMENT_DECLARE,
    ANT_BLOCK, ANT_ASSIGNMENT, ANT_UPDATE, ANT_INCDEC,
    ANT_WHILE, ANT_IF, ANT_FOR, ANT_SIMPLE, ANT_RETURN,
    ANT_INDEXING, ANT_ARGUMENT, ANT_FUNCALL, ANT_BINOP, ANT_UNOP, ANT_EXP_LEAF
};

class IAstNode {
public:
    virtual ~IAstNode() {}
    virtual AstNodeType GetType(void) = 0;
    virtual void Visit(IAstVisitor *visitor) = 0;
    virtual bool TreesAreEqual(IAstNode *other_tree) = 0;
};

/////////////////////////
//
// TYPES
//
/////////////////////////
class AstArgumentDecl : public IAstNode {
public:
    Token       direction_;
    string      name_;
    IAstNode    *type_;
    IAstNode    *initer_;

    virtual ~AstArgumentDecl() { if (type_ != NULL) delete type_; if (initer_ != NULL) delete initer_; }
    AstArgumentDecl(Token dir, const char *name) : direction_(dir), name_(name), type_(NULL), initer_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_ARGUMENT_DECLARE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddType(IAstNode *type) { type_ = type; }
    void AddIniter(IAstNode *initer) { initer_ = initer; }
};

class AstFuncType : public IAstNode {
public:
    bool                        ispure_;
    bool                        varargs_;
    vector<AstArgumentDecl*>    arguments_;
    IAstNode                    *return_type_;

    virtual ~AstFuncType();
    AstFuncType(bool ispure) : ispure_(ispure), varargs_(false), return_type_(NULL) { arguments_.reserve(8); }
    virtual AstNodeType GetType(void) { return(ANT_FUNC_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void SetVarArgs(void) { varargs_ = true; }
    void AddArgument(AstArgumentDecl *arg) { arguments_.push_back(arg); }
    void SetReturnType(IAstNode *type) { return_type_ = type; }
};

class AstPointerType : public IAstNode {
public:
    bool        isconst_;
    bool        isweak_;
    IAstNode    *pointed_type_;

    virtual ~AstPointerType() { if (pointed_type_ != NULL) delete pointed_type_; }
    AstPointerType(bool isconst, bool isweak, IAstNode *pointed) : isconst_(isconst), isweak_(isweak), pointed_type_(pointed) {}
    virtual AstNodeType GetType(void) { return(ANT_POINTER_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstMapType : public IAstNode {
public:
    IAstNode    *key_type_;
    IAstNode    *returned_type_;

    virtual ~AstMapType() { if (key_type_ != NULL) delete key_type_; if (returned_type_ != NULL) delete returned_type_; }
    AstMapType(IAstNode *key, IAstNode *rett) : key_type_(key), returned_type_(rett) {}
    virtual AstNodeType GetType(void) { return(ANT_MAP_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstArrayOrMatrixType : public IAstNode {
public:
    bool                is_matrix_;
    vector<int>         dimensions_;
    vector<IAstNode*>   expressions_;
    IAstNode            *element_type_;

    virtual ~AstArrayOrMatrixType();
    AstArrayOrMatrixType(bool ismatrix) : is_matrix_(ismatrix), element_type_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_ARRAY_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void SetDimensionValue(int value) { dimensions_.push_back(value); expressions_.push_back(NULL); }
    void SetDimensionExpression(IAstNode *exp) { dimensions_.push_back(-1); expressions_.push_back(exp); }
    void SetElementType(IAstNode *etype) { element_type_ = etype; }
};

class AstQualifiedType : public IAstNode {
public:
    NamesList  names_;

    AstQualifiedType(const char *name) { names_.AddName(name); }
    virtual AstNodeType GetType(void) { return(ANT_NAMED_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddNameComponent(const char *name) { names_.AddName(name); }
    void BuildTheFullName(string *dst);
};

class AstNamedType : public IAstNode {
public:
    string  name_;

    AstNamedType(const char *name) : name_(name) {}
    virtual AstNodeType GetType(void) { return(ANT_NAMED_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstBaseType : public IAstNode {
public:
    Token   base_type_;

    AstBaseType(Token token) { base_type_ = token; }
    virtual AstNodeType GetType(void) { return(ANT_BASE_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

/////////////////////////
//
// EXPRESSIONS
//
/////////////////////////
class AstExpressionLeaf : public IAstNode {
public:
    Token   subtype_;
    string  value_;          // a literal string representation or a variable/class/package/type name

    AstExpressionLeaf(Token type, const char *value) : subtype_(type), value_(value) {}
    virtual AstNodeType GetType(void) { return(ANT_EXP_LEAF); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

// includes (), *, cast, sizeof, dimof.
class AstUnop : public IAstNode {
public:
    Token       subtype_;
    IAstNode    *operand_;

    virtual ~AstUnop() { if (operand_ != NULL) delete operand_; }
    AstUnop(Token type, IAstNode *operand) : operand_(operand), subtype_(type) {}
    virtual AstNodeType GetType(void) { return(ANT_UNOP); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

// includes '.' (field access, scope resolution)
class AstBinop : public IAstNode {
public:
    Token       subtype_;
    IAstNode    *operand_left_;
    IAstNode    *operand_right_;

    virtual ~AstBinop() { if (operand_left_ != NULL) delete operand_left_; if (operand_right_ != NULL) delete operand_right_; }
    AstBinop(Token type, IAstNode *left, IAstNode*right) : subtype_(type), operand_left_(left), operand_right_(right) {}
    virtual AstNodeType GetType(void) { return(ANT_BINOP); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstFunCall : public IAstNode {
public:
    IAstNode            *left_term_;
    vector<IAstNode*>   arguments_;

    virtual ~AstFunCall();
    AstFunCall(IAstNode *left) : left_term_(left) {}
    virtual AstNodeType GetType(void) { return(ANT_FUNCALL); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddAnArgument(IAstNode *value) { arguments_.push_back(value); }
};

class AstArgument : public IAstNode {
public:
    string      name_;
    IAstNode    *expression_;
    IAstNode    *cast_to_;

    virtual ~AstArgument();
    AstArgument(IAstNode *value) : expression_(value), cast_to_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_ARGUMENT); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddName(const char *value) { name_ = value; }
    void CastTo(IAstNode *type) { cast_to_ = type; }
};

class AstIndexing : public IAstNode {
public:
    IAstNode            *left_term_;
    vector<IAstNode*>   lower_values_;
    vector<IAstNode*>   upper_values_;

    virtual ~AstIndexing();
    AstIndexing(IAstNode *left) : left_term_(left) {}
    virtual AstNodeType GetType(void) { return(ANT_INDEXING); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddAnIndex(IAstNode *value) { lower_values_.push_back(value); upper_values_.push_back(NULL); }
    void AddARange(IAstNode *lower, IAstNode *higher) { lower_values_.push_back(lower); upper_values_.push_back(higher); }
};

/////////////////////////
//
// STATEMENTS
//
/////////////////////////

class AstBlock : public IAstNode {
public:
    vector<IAstNode*>   block_items_;

    virtual ~AstBlock();
    virtual AstNodeType GetType(void) { return(ANT_BLOCK); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddItem(IAstNode *node) { block_items_.push_back(node); }
};

class AstIncDec : public IAstNode {
public:
    Token       operation_;
    IAstNode    *left_term_;

    virtual ~AstIncDec() { if (left_term_ != NULL) delete left_term_; }
    AstIncDec(Token op, IAstNode *left) : operation_(op), left_term_(left) {}
    virtual AstNodeType GetType(void) { return(ANT_INCDEC); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstUpdate : public IAstNode {
public:
    Token       operation_;
    IAstNode    *left_term_;
    IAstNode    *right_term_;

    virtual ~AstUpdate() { if (left_term_ != NULL) delete left_term_; if (right_term_ != NULL) delete right_term_; }
    AstUpdate(Token op, IAstNode *left, IAstNode *right) : operation_(op), left_term_(left), right_term_(right) {}
    virtual AstNodeType GetType(void) { return(ANT_UPDATE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstAssignment : public IAstNode {
public:
    vector<IAstNode*>   left_terms_;
    vector<IAstNode*>   right_terms_;

    virtual ~AstAssignment();
    AstAssignment(vector<IAstNode*> *left, vector<IAstNode*> *right) : left_terms_(*left), right_terms_(*right) {}
    virtual AstNodeType GetType(void) { return(ANT_ASSIGNMENT); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstWhile : public IAstNode {
public:
    IAstNode    *expression_;
    AstBlock    *block_;

    virtual ~AstWhile() { if (expression_ != NULL) delete expression_;  if (block_ != NULL) delete block_; }
    AstWhile(IAstNode *exp, AstBlock *blk) : expression_(exp), block_(blk) {}
    virtual AstNodeType GetType(void) { return(ANT_WHILE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
    IAstNode *GetTheExpression(void) { return(expression_); }
    IAstNode *GetTheBlock(void) { return(block_); }
};

class AstIf : public IAstNode {
public:
    vector<IAstNode*>   expressions_;
    vector<AstBlock*>   blocks_;
    AstBlock            *default_block_;

    virtual ~AstIf();
    AstIf() : default_block_(NULL) { expressions_.reserve(4); blocks_.reserve(4); }
    virtual AstNodeType GetType(void) { return(ANT_IF); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
    void AddExpression(IAstNode *exp) { expressions_.push_back(exp); }
    void AddBlock(AstBlock *blk) { blocks_.push_back(blk); }
    void SetDefaultBlock(AstBlock *blk) { default_block_ = blk;}
};

class AstFor : public IAstNode {
public:
    string      index_name_;
    string      iterator_name_;
    IAstNode    *set_;
    IAstNode    *low_;
    IAstNode    *high_;
    IAstNode    *step_;
    AstBlock    *block_;

    virtual ~AstFor();
    AstFor() : set_(NULL), low_(NULL), high_(NULL), step_(NULL), block_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_FOR); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
    void SetIndexName(const char *name) { index_name_ = name; }
    void SetIteratorName(const char *name) { iterator_name_ = name; }
    void SetTheSet(IAstNode *exp) { set_ = exp; }
    void SetLowBound(IAstNode *exp) { low_ = exp; }
    void SetHightBound(IAstNode *exp) { high_ = exp; }
    void SetStep(IAstNode *exp) { step_ = exp; }
    void SetBlock(AstBlock *block) { block_ = block; }
};

class AstSimpleStatement : public IAstNode {
public:
    Token   subtype_;

    AstSimpleStatement(Token type) : subtype_(type) {}
    virtual AstNodeType GetType(void) { return(ANT_SIMPLE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
};

class AstReturn : public IAstNode {
public:
    IAstNode *retvalue_;

    virtual ~AstReturn() { if (retvalue_ != NULL) delete retvalue_; }
    AstReturn(IAstNode *expression) : retvalue_(expression) {}
    virtual AstNodeType GetType(void) { return(ANT_RETURN); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
};

/////////////////////////
//
// BASE STRUCTURE OF PACKAGE
//
/////////////////////////
class AstIniter : public IAstNode {
public:
    vector<IAstNode*>   elements_;

    virtual ~AstIniter();
    AstIniter() { elements_.reserve(4); }
    virtual AstNodeType GetType(void) { return(ANT_INITER); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddElement(IAstNode *element) { elements_.push_back(element); }
};

class VarDeclaration : public IAstNode
{
public:
    string      name_;
    bool        volatile_flag_;
    IAstNode    *type_spec_;
    IAstNode    *initer_;

    virtual ~VarDeclaration() { if (type_spec_ != NULL) delete type_spec_; if (initer_ != NULL) delete initer_; }
    VarDeclaration(const char *name) : name_(name), volatile_flag_(false), type_spec_(NULL), initer_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_VAR); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void SetVolatile(void) { volatile_flag_ = true; }
    void SetType(IAstNode *node) { type_spec_ = node; }
    void SetIniter(IAstNode *node) { initer_ = node; }
};

class ConstDeclaration : public IAstNode
{
public:
    string      name_;
    IAstNode    *type_spec_;
    IAstNode    *initer_;

    virtual ~ConstDeclaration() { if (type_spec_ != NULL) delete type_spec_; if (initer_ != NULL) delete initer_; }
    ConstDeclaration(const char *name) : name_(name), type_spec_(NULL), initer_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_CONST); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
    void SetType(IAstNode *node) { type_spec_ = node; }
    void SetIniter(IAstNode *node) { initer_ = node; }
};

class TypeDeclaration : public IAstNode
{
public:
    string      name_;
    IAstNode    *type_spec_;

    virtual ~TypeDeclaration() { if (type_spec_ != NULL) delete type_spec_; }
    TypeDeclaration(const char *name) : name_(name), type_spec_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree) { return(false); }
    void SetType(IAstNode *node) { type_spec_ = node; }
};

class FuncDeclaration : public IAstNode
{
public:
    string      name_;
    bool        is_class_member_;
    string      classname_;
    AstFuncType *function_type_;
    AstBlock    *block_;

    virtual ~FuncDeclaration() { if (function_type_ != NULL) delete function_type_; if (block_ != NULL) delete block_; }
    FuncDeclaration(const char *name1, const char *name2);
    virtual AstNodeType GetType(void) { return(ANT_FUNC); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddType(AstFuncType *type) { function_type_ = type; }
    void AddBlock(AstBlock *block) { block_ = block; }
};

class AstDependency : public IAstNode {
public:
    string package_dir_;
    string package_name_;

    //virtual ~AstDependency();
    AstDependency(const char *path, const char *name);
    virtual AstNodeType GetType(void) { return(ANT_DEPENDENCY); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstFile : public IAstNode {
public:
    string                  package_name_;
    vector<AstDependency*>  dependencies_;
    vector<IAstNode*>       declarations_;

    virtual ~AstFile();
    AstFile(const char *package) : package_name_(package) {
        dependencies_.reserve(8); 
        declarations_.reserve(16);
    }
    virtual AstNodeType GetType(void) { return(ANT_FILE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddDependency(AstDependency *dep) { dependencies_.push_back(dep); }
    void AddNode(IAstNode *node) { declarations_.push_back(node); }
};

};

#endif