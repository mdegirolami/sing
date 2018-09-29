#ifndef AST_NODES_H
#define AST_NODES_H

#include "string"
#include "lexer.h"

namespace StayNames {

class IAstVisitor {
public:
    // base structure
    virtual void File(const char *package) = 0;
    virtual void PackageRef(const char *path, const char *package_name) = 0;
    virtual void BeginVarDeclaration(const char *name, bool isvolatile, bool has_initer) = 0;
    virtual void EndVarDeclaration(const char *name, bool isvolatile, bool has_initer) = 0;
    virtual void BeginFuncDeclaration(const char *name, bool ismember, const char *classname) = 0;
    virtual void EndFuncDeclaration(const char *name, bool ismember, const char *classname) = 0;

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

    // expressions
    virtual void ExpLeaf(Token type, const char *value) = 0;
    virtual void BeginUnop(Token subtype) = 0;
    virtual void EndUnop(Token subtype) = 0;
    virtual void BeginBinop(Token subtype) = 0;
    virtual void BeginBinopSecondArg(void) = 0;
    virtual void EndBinop(Token subtype) = 0;

    // types
    virtual void BeginFuncType(bool ispure_, bool varargs_, int num_args) = 0;
    virtual void EndFuncType(bool ispure_, bool varargs_, int num_args) = 0;
    virtual void BeginArgumentDecl(Token direction, const char *name, bool has_initer) = 0;
    virtual void EndArgumentDecl(Token direction, const char *name, bool has_initer) = 0;
    virtual void BeginArrayOrMatrixType(bool is_matrix_, int dimensions_count) = 0;
    virtual void EndArrayOrMatrixType(bool is_matrix_, int dimensions_count) = 0;
    virtual void ConstIntExpressionValue(int value) = 0;
    virtual void NameOfType(const char *package, const char *name) = 0; // package may be NULL
    virtual void BaseType(Token token) = 0;
};

enum AstNodeType {
    ANT_FILE, ANT_DEPENDENCY, ANT_VAR, ANT_CONST, ANT_TYPE, ANT_INITER, ANT_FUNC,
    ANT_BASE_TYPE, ANT_NAMED_TYPE, ANT_QUALIFIED_TYPE, ANT_ARRAY_TYPE, ANT_MAP_TYPE, ANT_POINTER_TYPE, ANT_FUNC_TYPE,
    ANT_ARGUMENT_DECLARE,
    ANT_BLOCK, ANT_ASSIGNMENT, ANT_UPDATE, ANT_INCDEC,
    ANT_INDEXING, ANT_ARGUMENT, ANT_NAMED_ARG, ANT_FUNCALL, ANT_BINOP, ANT_UNOP, ANT_EXP_LEAF
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
    Token       direction_;
    string      name_;
    IAstNode    *type_;
    IAstNode    *initer_;

public:
    virtual ~AstArgumentDecl() { if (type_ != NULL) delete type_; if (initer_ != NULL) delete initer_; }
    AstArgumentDecl(Token dir, const char *name) : direction_(dir), name_(name), type_(NULL), initer_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_ARGUMENT_DECLARE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddType(IAstNode *type) { type_ = type; }
    void AddIniter(IAstNode *initer) { initer_ = initer; }
};

class AstFuncType : public IAstNode {
    bool                        ispure_;
    bool                        varargs_;
    vector<AstArgumentDecl*>    arguments_;
    IAstNode                    *return_type_;

public:
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
    bool        isconst;
    bool        isweak;
    IAstNode    *next;

    virtual ~AstPointerType() { if (next != NULL) delete next; }
    virtual AstNodeType GetType(void) { return(ANT_POINTER_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstMapType : public IAstNode {
public:
    IAstNode    *key_type;
    IAstNode    *next;

    virtual ~AstMapType() { if (key_type != NULL) delete key_type; if (next != NULL) delete next; }
    virtual AstNodeType GetType(void) { return(ANT_MAP_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstArrayOrMatrixType : public IAstNode {
    bool                is_matrix_;
    vector<int>         dimensions_;
    vector<IAstNode*>   expressions_;
    IAstNode            *element_type_;

public:
    virtual ~AstArrayOrMatrixType() { if (element_type_ != NULL) delete element_type_; }
    AstArrayOrMatrixType(bool ismatrix) : is_matrix_(ismatrix), element_type_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_ARRAY_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstQualifiedType : public IAstNode {
    string  name_;
    string  package_;

public:
    AstQualifiedType(const char *pkg, const char *name) : name_(name), package_(pkg) {}
    virtual AstNodeType GetType(void) { return(ANT_QUALIFIED_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstNamedType : public IAstNode {
    string  name_;

public:
    AstNamedType(const char *name) : name_(name) {}
    virtual AstNodeType GetType(void) { return(ANT_NAMED_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstBaseType : public IAstNode {
    Token   base_type_;

public:
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
    Token   subtype_;
    string  value_;          // a literal string representation or a variable/class/package/type name

public:
    AstExpressionLeaf(Token type, const char *value) : subtype_(type), value_(value) {}
    virtual AstNodeType GetType(void) { return(ANT_EXP_LEAF); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

// includes (), *, cast, sizeof, dimof.
class AstUnop : public IAstNode {
    Token       subtype_;
    IAstNode    *operand_;

public:
    virtual ~AstUnop() { if (operand_ != NULL) delete operand_; }
    AstUnop(Token type, IAstNode *operand) : operand_(operand), subtype_(type) {}
    virtual AstNodeType GetType(void) { return(ANT_UNOP); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

// includes '.' (field access, scope resolution)
class AstBinop : public IAstNode {
    Token       subtype_;
    IAstNode    *operand_left_;
    IAstNode    *operand_right_;

public:
    virtual ~AstBinop() { if (operand_left_ != NULL) delete operand_left_; if (operand_right_ != NULL) delete operand_right_; }
    AstBinop(Token type, IAstNode *left, IAstNode*right) : subtype_(type), operand_left_(left), operand_right_(right) {}
    virtual AstNodeType GetType(void) { return(ANT_BINOP); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstFunCall : public IAstNode {
public:
    IAstNode    *left_term;
    IAstNode    *arguments;

    virtual ~AstFunCall() { if (left_term != NULL) delete left_term; if (arguments != NULL) delete arguments; }
    virtual AstNodeType GetType(void) { return(ANT_FUNCALL); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstArgument : public IAstNode {
public:
    IAstNode    *expression;
    IAstNode    *cast_to;
    AstArgument *next;

    virtual ~AstArgument();
    virtual AstNodeType GetType(void) { return(ANT_ARGUMENT); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstNamedArgument : public IAstNode {
public:
    string      name;
    IAstNode    *expression;
    IAstNode    *cast_to;
    AstArgument *next;

    virtual ~AstNamedArgument();
    virtual AstNodeType GetType(void) { return(ANT_NAMED_ARG); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstIndexing : public IAstNode {
public:
    IAstNode            *left_term;
    vector<IAstNode*>   expressions;
    vector<IAstNode*>   upper_values;

    virtual ~AstIndexing();
    virtual AstNodeType GetType(void) { return(ANT_INDEXING); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

/////////////////////////
//
// STATEMENTS
//
/////////////////////////
class AstIncDec : public IAstNode {
    Token       operation_;
    IAstNode    *left_term_;

public:
    virtual ~AstIncDec() { if (left_term_ != NULL) delete left_term_; }
    AstIncDec(Token op, IAstNode *left) : operation_(op), left_term_(left) {}
    virtual AstNodeType GetType(void) { return(ANT_INCDEC); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstUpdate : public IAstNode {
    Token       operation_;
    IAstNode    *left_term_;
    IAstNode    *right_term_;

public:
    virtual ~AstUpdate() { if (left_term_ != NULL) delete left_term_; if (right_term_ != NULL) delete right_term_; }
    AstUpdate(Token op, IAstNode *left, IAstNode *right) : operation_(op), left_term_(left), right_term_(right) {}
    virtual AstNodeType GetType(void) { return(ANT_UPDATE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstAssignment : public IAstNode {
    vector<IAstNode*>   left_terms_;
    vector<IAstNode*>   right_terms_;

public:
    virtual ~AstAssignment();
    AstAssignment(vector<IAstNode*> *left, vector<IAstNode*> *right) : left_terms_(*left), right_terms_(*right) {}
    virtual AstNodeType GetType(void) { return(ANT_ASSIGNMENT); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstBlock : public IAstNode {
    vector<IAstNode*>   block_items_;

public:
    virtual ~AstBlock();
    virtual AstNodeType GetType(void) { return(ANT_BLOCK); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddItem(IAstNode *node) { block_items_.push_back(node); }
};

/////////////////////////
//
// BASE STRUCTURE OF PACKAGE
//
/////////////////////////
class AstIniter : public IAstNode {
public:
    virtual ~AstIniter() {}
    virtual AstNodeType GetType(void) { return(ANT_INITER); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class VarDeclaration : public IAstNode
{
    string          name_;
    bool            volatile_flag_;
    IAstNode        *type_spec_;
    AstIniter       *initer_;

public:
    virtual ~VarDeclaration() { if (type_spec_ != NULL) delete type_spec_; if (initer_ != NULL) delete initer_; }
    VarDeclaration(const char *name) : name_(name), volatile_flag_(false), type_spec_(NULL), initer_(NULL) {}
    virtual AstNodeType GetType(void) { return(ANT_VAR); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void SetVolatile(void) { volatile_flag_ = true; }
    void SetType(IAstNode *node) { type_spec_ = node; }
    void SetIniter(AstIniter *node) { initer_ = node; }
};

class FuncDeclaration : public IAstNode
{
    string      name_;
    bool        is_class_member_;
    string      classname_;
    AstFuncType *function_type_;
    AstBlock    *block_;

public:
    virtual ~FuncDeclaration() { if (function_type_ != NULL) delete function_type_; if (block_ != NULL) delete block_; }
    FuncDeclaration(const char *name1, const char *name2);
    virtual AstNodeType GetType(void) { return(ANT_FUNC); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
    void AddType(AstFuncType *type) { function_type_ = type; }
    void AddBlock(AstBlock *block) { block_ = block; }
};

class AstDependency : public IAstNode {
    string package_dir_;
    string package_name_;

public:
    //virtual ~AstDependency();
    AstDependency(const char *path, const char *name);
    virtual AstNodeType GetType(void) { return(ANT_DEPENDENCY); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstFile : public IAstNode {
    string                  package_name_;
    vector<AstDependency*>  dependencies_;
    vector<IAstNode*>       declarations_;

public:
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