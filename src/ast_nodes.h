#ifndef AST_NODES_H
#define AST_NODES_H

#include "string"
#include "lexer.h"

namespace StayNames {

class IAstVisitor {
public:
    virtual void File(const char *package) = 0;
    virtual void PackageRef(const char *path, const char *package_name) = 0;
    virtual void StartDeclaration(Token type) = 0;
};

enum AstNodeType {
    ANT_FILE, ANT_DEPENDENCY, ANT_VAR, ANT_CONST, ANT_TYPE, ANT_INITER, ANT_FUNC,
    ANT_BASE_TYPE, ANT_NAMED_TYPE, ANT_QUALIFIED_TYPE, ANT_ARRAY_TYPE, ANT_MAP_TYPE, ANT_POINTER_TYPE, ANT_FPTR_TYPE,
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
class AstFuncPointerType : public IAstNode {
public:
    bool            ispure;
    bool            varargs;
    IAstArgument    *arg;
    IAstNode        *return_type;

    virtual ~AstFuncPointerType() { if (arg != NULL) delete arg; if (return_type != NULL) delete return_type; }
    virtual AstNodeType GetType(void) { return(ANT_FPTR_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class IAstArgument : public IAstNode {
public:
    Token       direction;
    string      name;
    IAstNode    *type;
    IAstNode    *initer;

    virtual ~IAstArgument() { if (type != NULL) delete type; if (initer != NULL) delete initer; }
    virtual AstNodeType GetType(void) { return(ANT_ARGUMENT_DECLARE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
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
public:
    bool            is_matrix;
    vector<int>     dimensions;
    IAstNode        *next;

    virtual ~AstArrayOrMatrixType() { if (next != NULL) delete next; }
    virtual AstNodeType GetType(void) { return(ANT_ARRAY_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstQualifiedType : public IAstNode {
public:
    string  name;
    string  package;

    virtual AstNodeType GetType(void) { return(ANT_QUALIFIED_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstNamedType : public IAstNode {
public:
    string  name;

    virtual AstNodeType GetType(void) { return(ANT_NAMED_TYPE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstBaseType : public IAstNode {
public:
    Token   base_type;

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
    Token   subtype;
    string  value;          // a literal string representation or a variable/class/package/type name

    virtual AstNodeType GetType(void) { return(ANT_EXP_LEAF); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

// includes (), *, cast, sizeof, dimof.
class AstUnop : public IAstNode {
public:
    Token       subtype;
    IAstNode    *operand;

    virtual ~AstUnop() { if (operand != NULL) delete operand; }
    virtual AstNodeType GetType(void) { return(ANT_UNOP); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

// includes '.' (field access, scope resolution)
class AstBinop : public IAstNode {
public:
    Token       subtype;
    IAstNode    *operand_left;
    IAstNode    *operand_right;

    virtual ~AstBinop() { if (operand_left != NULL) delete operand_left; if (operand_right != NULL) delete operand_right; }
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
public:
    Token       operation;
    IAstNode    *left_term;

    virtual ~AstIncDec() { if (left_term != NULL) delete left_term; }
    virtual AstNodeType GetType(void) { return(ANT_INCDEC); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstUpdate : public IAstNode {
public:
    Token       operation;
    IAstNode    *left_term;
    IAstNode    *right_term;

    virtual ~AstUpdate() { if (left_term != NULL) delete left_term; if (right_term != NULL) delete right_term; }
    virtual AstNodeType GetType(void) { return(ANT_UPDATE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstAssignment : public IAstNode {
public:
    vector<IAstNode*>   left_terms;
    vector<IAstNode*>   right_terms;

    virtual ~AstAssignment();
    virtual AstNodeType GetType(void) { return(ANT_ASSIGNMENT); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstBlock : public IAstNode {
public:
    IAstNode    *block_items;
    IAstNode    *next;

    virtual ~AstBlock() { if (block_items != NULL) delete block_items; if (next != NULL) delete next; }
    virtual AstNodeType GetType(void) { return(ANT_BLOCK); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
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
public:
    string          name;
    bool            volatile_flag;
    IAstNode        *type_spec;
    AstIniter       *initer;

    virtual ~VarDeclaration() { if (type_spec != NULL) delete type_spec; if (initer != NULL) delete initer; }
    virtual AstNodeType GetType(void) { return(ANT_VAR); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class FuncDeclaration : public IAstNode
{
public:
    string      name;
    bool        is_class_member;
    string      classname;
    IAstNode    *type_spec;
    AstBlock    *block;

    virtual ~FuncDeclaration() { if (type_spec != NULL) delete type_spec; if (block != NULL) delete block; }
    virtual AstNodeType GetType(void) { return(ANT_FUNC); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstDependency : public IAstNode {
public:
    string package_name;
    string package_dir;
    AstDependency *next;

    virtual ~AstDependency() { if (next != NULL) delete next; }
    virtual AstNodeType GetType(void) { return(ANT_DEPENDENCY); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

class AstFile : public IAstNode {
public:
    string              package_name;
    AstDependency       *dependencies;
    vector<IAstNode*>   declarations;

    virtual ~AstFile();
    virtual AstNodeType GetType(void) { return(ANT_FILE); }
    virtual void Visit(IAstVisitor *visitor);
    virtual bool TreesAreEqual(IAstNode *other_tree);
};

};

#endif