#ifndef AST_NODES_H
#define AST_NODES_H

#include "string"
#include "lexer.h"
#include "NamesList.h"
#include "expression_attributes.h"
#include "target.h"

//
// Adding a new node
//
// Declare the node as inheriting from IAstNode, IAstTypeNode, IAstExpNode or IAstDeclarationNode
// add a new value to NodeType enum
// make a RAII constructor
// make a destructor which deletes the childrens
// implement IAstNode functions
// add other construction routines as needed 
// 
namespace SingNames {

// coommon to vars consts arguments for iterators/indices.
enum VarFlags {
    VF_READONLY = 1,    // consts, in arguments, for iterators/indices
    VF_WRITEONLY = 2,   // es: out arguments
    VF_WASREAD = 4,     // to determine args direction and if an item is actually used
    VF_WASWRITTEN = 8,

    VF_ISPOINTED = 0x10,    // do we need to allocate on the heap ?

    VF_ISARG = 0x100,
    VF_ISBUSY = 0x200,      // applies to iterators/indices inside the for block
    VF_ISFORINDEX = 0x400,
    VF_ISFORITERATOR = 0x800,
    VF_IS_REFERENCE = 0x1000,   // for iterator or typeswitch placement name
    VF_ISLOCAL = 0x2000,
    VF_IMPLEMENTED_AS_CONSTINT = 0x4000, // is implemented as a static const int (run time constant).
    VF_IS_ITERATED = 0x8000
};

enum AstNodeType {

    // top level declartions
    ANT_FILE, ANT_DEPENDENCY, ANT_VAR, ANT_TYPE, ANT_INITER, ANT_FUNC, 

    // types description
    ANT_BASE_TYPE, ANT_NAMED_TYPE, ANT_ARRAY_TYPE, ANT_MAP_TYPE, ANT_POINTER_TYPE, ANT_FUNC_TYPE, 
    ANT_CLASS_TYPE, ANT_INTERFACE_TYPE, ANT_ENUM_TYPE,

    // statements
    ANT_BLOCK, ANT_UPDATE, ANT_INCDEC, ANT_SWAP,
    ANT_WHILE, ANT_IF, ANT_FOR, ANT_SIMPLE, ANT_RETURN,
    ANT_SWITCH, ANT_TYPESWITCH,

    // expressions
    ANT_INDEXING, ANT_ARGUMENT, ANT_FUNCALL, ANT_BINOP, ANT_UNOP, ANT_EXP_LEAF
};

struct PositionInfo {
    int32_t start_row;
    int32_t start_col;
    int32_t end_row;        // end of portion to be highlit on error (typically the main token)
    int32_t end_col;
    int32_t last_row;       // end of the node and its children - used to attribute remarks
    int32_t last_col;

    PositionInfo()
    {
        start_row = 0;
        start_col = 0;
        end_row = 0;   
        end_col = 0;
        last_row = 0;  
        last_col = 0;
    }
};

enum class RdType { EMPTY_LINE, COMMENT_ONLY, COMMENT_AFTER_CODE };

struct RemarkDescriptor {
    string          remark;
    int             row;
    int             col;
    RdType          rd_type;
};

class IAstNode {
public:
    virtual ~IAstNode() {}
    virtual AstNodeType GetType(void) const = 0;
    virtual PositionInfo *GetPositionRecord(void) = 0;
    virtual bool IsARemarkableNode(void) = 0;   // i.e: can accept attached comments
};

enum TypeComparisonMode {FOR_ASSIGNMENT,            // returns true if you can copy type src_tree to 'this'
                         FOR_EQUALITY,              // compare 'this' and src_tree. Just ignores very secondary stuff. (this is the stricter !!)
                         FOR_REFERENCING};          // true if 'this' can be passed as argument to a function whose argument is an output declared as src_tree.

enum ForwardReferenceType {FRT_NONE, FRT_PRIVATE, FRT_PUBLIC};

enum ParmPassingMethod { PPM_VALUE, PPM_POINTER, PPM_CONSTREF, PPM_INPUT_STRING };

class IAstTypeNode : public IAstNode {
public:
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode) = 0;    // shallow compare !!
    virtual int SizeOf(void) = 0;
    virtual bool NeedsZeroIniter(void) = 0;
    virtual bool SupportsEqualOperator(void) = 0;
};

const IAstTypeNode *SolveTypedefs(const IAstTypeNode *begin);
IAstTypeNode *SolveTypedefs(IAstTypeNode *begin);
ParmPassingMethod GetParameterPassingMethod(IAstTypeNode *type_spec, bool input_parm);

class IAstExpNode : public IAstNode {
public:
    virtual const ExpressionAttributes *GetAttr(void) = 0;
    virtual bool HasFunction(void) = 0;
};

class IAstDeclarationNode : public IAstNode {
public:
    virtual bool IsPublic(void) = 0;
    virtual void SetPublic(bool value) = 0;
};

class TypeDeclaration;
class VarDeclaration;
class FuncDeclaration;

/////////////////////////
//
// TYPES
//
/////////////////////////
class AstFuncType : public IAstTypeNode {
public:
    bool                        ispure_;
    bool                        varargs_;
    bool                        is_member_;
    vector<VarDeclaration*>     arguments_;
    IAstTypeNode                *return_type_;
    PositionInfo                pos_;
    bool                        is_owning_;     // i.e. pointer to return_type is owning.

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstFuncType();
    AstFuncType(bool ispure) : ispure_(ispure), varargs_(false), return_type_(NULL), is_member_(false), is_owning_(true) { arguments_.reserve(8); }
    virtual AstNodeType GetType(void) const { return(ANT_FUNC_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void) { return(KPointerSize); }
    virtual bool NeedsZeroIniter(void) { return(true); }
    virtual bool SupportsEqualOperator(void) { return(true); }
    void SetVarArgs(void) { varargs_ = true; }
    void AddArgument(VarDeclaration *arg) { arguments_.push_back(arg); }
    void SetReturnType(IAstTypeNode *type) { return_type_ = type; }
    bool ReturnsVoid(void);
    void SetIsMember(void) { is_member_ = true; }
    void SetOwning(bool value) { is_owning_ = value; }
};

class AstPointerType : public IAstTypeNode {
public:
    bool            isconst_;
    bool            isweak_;
    IAstTypeNode    *pointed_type_;
    PositionInfo    pos_;
    bool            owning_;         // annotation: own pointed_type_ ? (need non-owning pointers to declare auto vars from the address-of initializers).

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstPointerType() { if (pointed_type_ != NULL && owning_) delete pointed_type_; }
    AstPointerType() : isconst_(false), isweak_(false), pointed_type_(nullptr), owning_(true) {}
    virtual AstNodeType GetType(void) const { return(ANT_POINTER_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void) { return(KPointerSize); }
    virtual bool NeedsZeroIniter(void) { return(false); }
    virtual bool SupportsEqualOperator(void) { return(!isweak_); }
    bool CheckConstness(IAstTypeNode *src_tree, TypeComparisonMode mode);
    void Set(bool isconst, bool isweak, IAstTypeNode *pointed) { isconst_ = isconst; isweak_ = isweak; pointed_type_ = pointed; }
    void SetWithRef(bool isconst, IAstTypeNode *pointed) { isconst_ = isconst; owning_ = false; pointed_type_ = pointed; }
};

class AstMapType : public IAstTypeNode {
public:
    IAstTypeNode    *key_type_;
    IAstTypeNode    *returned_type_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstMapType() { if (key_type_ != NULL) delete key_type_; if (returned_type_ != NULL) delete returned_type_; }
    AstMapType() : key_type_(NULL), returned_type_(NULL) {}
    virtual AstNodeType GetType(void) const { return(ANT_MAP_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void) { return(0); }
    virtual bool NeedsZeroIniter(void) { return(false); }
    virtual bool SupportsEqualOperator(void) { return(false); }
    void SetKeyType(IAstTypeNode *key) { key_type_ = key; }
    void SetReturnType(IAstTypeNode *return_type) { returned_type_ = return_type; }
};

class AstArrayType : public IAstTypeNode {
public:
    // [*] : is_dynamic_ == true, expression_ == nullptr
    // []  : is_dynamic_ == false, expression_ == nullptr
    // [expr] : is_dynamic_ == false, expression_ != nullptr
    bool            is_dynamic_;    // distinguishes [*] from []
    bool            is_regular;     // this + element_type_ are part of the same regular matrix (opposed to a jagged matrix)
    IAstExpNode     *expression_;
    IAstTypeNode    *element_type_;
    PositionInfo    pos_;

    // annotations
    // value is 0 if: [*]. 
    //                [expr], before CheckArrayIndicesInTypes() execution. 
    //                For [], before CheckArrayIniter() execution. 
    size_t          dimension_;
    bool            dimension_was_computed_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstArrayType();
    AstArrayType() : is_dynamic_(false), element_type_(NULL), expression_(NULL), 
                     dimension_(0), is_regular(false), dimension_was_computed_(false)
                     {}
    virtual AstNodeType GetType(void) const { return(ANT_ARRAY_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void) { return(0); }
    virtual bool NeedsZeroIniter(void) { return(false); }
    virtual bool SupportsEqualOperator(void);
    void SetDimensionExpression(IAstExpNode *exp) { dimension_ = 0; expression_ = exp; }
    void SetElementType(IAstTypeNode *etype) { element_type_ = etype; }
    void SetDynamic(bool dyna) { is_dynamic_ = dyna; }
    void SetRegular(bool regular) { is_regular = regular; }
};

class AstNamedType : public IAstTypeNode {
public:
    string          name_;
    PositionInfo    pos_;
    AstNamedType    *next_component;

    int                 pkg_index_;
    TypeDeclaration     *wp_decl_;   // annotation: the declaration the name refers to. Prevents the need to build a dictionary in the synth phase

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    AstNamedType(const char *name) : name_(name), wp_decl_(NULL), pkg_index_(-1), next_component(NULL) {}
    ~AstNamedType() { if (next_component != NULL) delete next_component; }
    virtual AstNodeType GetType(void) const { return(ANT_NAMED_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void);
    virtual bool NeedsZeroIniter(void);
    virtual bool SupportsEqualOperator(void);
    void ChainComponent(AstNamedType *next) { next_component = next; }
    void AppendFullName(string *fullname);  // for the purpose of emitting error messages
};

class AstBaseType : public IAstTypeNode {
public:
    Token           base_type_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    AstBaseType(Token token) { base_type_ = token; }
    virtual AstNodeType GetType(void) const { return(ANT_BASE_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void);
    virtual bool NeedsZeroIniter(void) { 
        return(base_type_ != TOKEN_COMPLEX64 &&
                base_type_ != TOKEN_COMPLEX128 &&
                base_type_ != TOKEN_STRING);
    }
    virtual bool SupportsEqualOperator(void) { return(true); }
};

class AstEnumType : public IAstTypeNode
{
public:
    vector<string>          items_;
    vector<IAstExpNode*>    initers_;
    PositionInfo            pos_;

    vector<int32_t>         indices_;           // annotations

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstEnumType();
    //AstEnumType() {}
    virtual AstNodeType GetType(void) const { return(ANT_ENUM_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void);
    virtual bool NeedsZeroIniter(void) { return(true); }
    virtual bool SupportsEqualOperator(void) { return(true); }
    void AddItem(const char *name, IAstExpNode *initer) { items_.push_back(name); initers_.push_back(initer); }
};

class AstInterfaceType : public IAstTypeNode
{
public:
    vector<AstNamedType*>       ancestors_;
    vector<FuncDeclaration*>    members_;               // annotations: this is grown with inherited functions
    PositionInfo                pos_;

    int                         first_hinherited_member_;   // annotations

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstInterfaceType();
    AstInterfaceType() : first_hinherited_member_(-1) {}
    virtual AstNodeType GetType(void) const { return(ANT_INTERFACE_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void);
    virtual bool NeedsZeroIniter(void) { return(false); }
    virtual bool SupportsEqualOperator(void) { return(false); }
    void AddAncestor(AstNamedType *anc) { ancestors_.push_back(anc); }
    void AddMember(FuncDeclaration *member) { members_.push_back(member); }
    bool HasInterface(AstInterfaceType *intf);
};

class AstClassType : public IAstTypeNode
{
public:
    vector<VarDeclaration*>     member_vars_;
    vector<FuncDeclaration*>    member_functions_;      // annotations: this is grown with inherited functions
    vector<string>              fn_implementors_;       // points into  member_vars_: this is grown with inherited functions
    vector<AstNamedType*>       member_interfaces_;
    vector<string>              if_implementors_;       // points into  member_vars_
    PositionInfo                pos_;

    int                     first_hinherited_member_;   // annotations
    vector<bool>            implemented_;
    bool                    has_destructor;
    bool                    can_be_copied;
    bool                    has_constructor;            // annotation from synthesizer
    bool                    constructor_written;        // annotation from synthesizer

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstClassType();
    AstClassType();
    virtual AstNodeType GetType(void) const { return(ANT_CLASS_TYPE); }
    virtual bool IsCompatible(IAstTypeNode *src_tree, TypeComparisonMode mode);
    virtual int SizeOf(void);
    virtual bool NeedsZeroIniter(void) { return(false); }
    virtual bool SupportsEqualOperator(void) { return(false); }
    void AddMemberVar(VarDeclaration *member) { 
        member_vars_.push_back(member); 
    }
    void AddMemberFun(FuncDeclaration *member, string implementor);
    void AddMemberInterface(AstNamedType *member, string implementor) {
        member_interfaces_.push_back(member);
        if_implementors_.push_back(implementor);
    }
    bool HasInterface(AstInterfaceType *intf);
    void SetNeedsConstructor(void) { has_constructor = true; }
    void SetConstructorDone(void) { constructor_written = true; }
    void DisableCopy(void) { can_be_copied = false; }
};

/////////////////////////
//
// EXPRESSIONS
//
/////////////////////////

class AstArgument : public IAstNode {
public:
    string          name_;
    IAstExpNode     *expression_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstArgument();
    AstArgument() : expression_(nullptr) {}
    virtual AstNodeType GetType(void) const { return(ANT_ARGUMENT); }
    void AddName(const char *value) { name_ = value; }
    void SetExpression(IAstExpNode *exp) { expression_ = exp; }
};

class AstExpressionLeaf : public IAstExpNode {
public:
    Token           subtype_;           // TOKEN_NAME, TOKEN_NULL, a boolean const or a numeric type (>= 32 bits).
    string          value_;             // a literal string representation or a variable/class/package/type name
    string          img_value_;         // a literal string representation of the imaginary part of a composite literal
    bool            real_is_int_;       // real is an int (img is always a float) 
    bool            real_is_negated_;   // real must be negated before use.
    bool            img_is_negated_;    // img must be negated before use.
    PositionInfo    pos_;

    int                 pkg_index_; 
    IAstDeclarationNode *wp_decl_;  // annotation: if subtype == TOKEN_NAME, the declaration the name refers to. (Func or Var)
                                    // Prevents the need to build a dictionary in the synth phase
    bool                unambiguous_member_access;

    ExpressionAttributes attr_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }
    virtual const ExpressionAttributes *GetAttr(void) { return(&attr_); }

    AstExpressionLeaf(Token type, const char *value);
    void SetImgValue(const char *value, bool is_negated) { img_value_ = value; img_is_negated_ = is_negated; }
    void SetRealPartNfo(bool isint, bool isnegated) { real_is_int_ = isint; real_is_negated_ = isnegated; }
    virtual AstNodeType GetType(void) const { return(ANT_EXP_LEAF); }
    void AppendToValue(const char *to_append) { value_ += to_append; }
    void SetUMA(bool value) { unambiguous_member_access = value; }
    virtual bool HasFunction(void) { return(false); }
};

// includes (), *, cast, sizeof, dimof.
class AstUnop : public IAstExpNode {
public:
    Token           subtype_;
    IAstExpNode     *operand_;
    IAstTypeNode    *type_;     // for sizeof(type)
    PositionInfo    pos_;

    ExpressionAttributes attr_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }
    virtual const ExpressionAttributes *GetAttr(void) { return(&attr_); }

    virtual ~AstUnop() { if (operand_ != NULL) delete operand_;  if (type_ != NULL) delete type_;  }
    AstUnop(Token type) : operand_(NULL), subtype_(type), type_(NULL) {}
    virtual AstNodeType GetType(void) const { return(ANT_UNOP); }
    void SetOperand(IAstExpNode *op) { operand_ = op; }
    void SetTypeOperand(IAstTypeNode *op) { type_ = op; }
    virtual bool HasFunction(void) { return(operand_ != nullptr && operand_->HasFunction()); }
};

// includes '.' (field access, scope resolution)
class AstBinop : public IAstExpNode {
public:
    Token           subtype_;
    IAstExpNode    *operand_left_;
    IAstExpNode    *operand_right_;
    PositionInfo    pos_;

    // attributes
    ExpressionAttributes attr_;
    AstFuncType     *builtin_;      // if this dotop is followed by a builtin function, this is the type (pointer is owning !). 
    const char      *builtin_signature_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }
    virtual const ExpressionAttributes *GetAttr(void) { return(&attr_); }

    AstBinop(Token type, IAstExpNode *left, IAstExpNode*right);
    virtual ~AstBinop();
    virtual AstNodeType GetType(void) const { return(ANT_BINOP); }
    void SetSignature(const char *signature) { builtin_signature_ = signature; }
    virtual bool HasFunction(void);
};

class AstFunCall : public IAstExpNode {
public:
    IAstExpNode             *left_term_;
    vector<AstArgument*>    arguments_;
    PositionInfo            pos_;
    bool                    is_statement_;

    AstFuncType          *func_type_;     // annotations
    ExpressionAttributes attr_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(is_statement_); }
    virtual const ExpressionAttributes *GetAttr(void) { return(&attr_); }

    virtual ~AstFunCall();
    AstFunCall(IAstExpNode *left) : left_term_(left), func_type_(nullptr), is_statement_(false) {}
    virtual AstNodeType GetType(void) const { return(ANT_FUNCALL); }
    void AddAnArgument(AstArgument *value) { arguments_.push_back(value); }
    virtual bool HasFunction(void) { return(true); }
    virtual bool FlagAsStatement(void) { is_statement_ = true; } 
};

class AstIndexing : public IAstExpNode {
public:
    IAstExpNode   *indexed_term_;
    IAstExpNode   *lower_value_;
    IAstExpNode   *upper_value_;
    bool           is_single_index_;
    PositionInfo   pos_;

    AstMapType           *map_type_;     // annotations
    ExpressionAttributes attr_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }
    virtual const ExpressionAttributes *GetAttr(void) { return(&attr_); }

    virtual ~AstIndexing();
    AstIndexing(IAstExpNode *left) : indexed_term_(left), lower_value_(nullptr), upper_value_(nullptr), map_type_(nullptr) {}
    virtual AstNodeType GetType(void) const { return(ANT_INDEXING); }
    void SetAnIndex(IAstExpNode *value) { 
        lower_value_ = value; 
        upper_value_ = nullptr; 
        is_single_index_ = true; 
    }
    void SetARange(IAstExpNode *lower, IAstExpNode *higher) { 
        lower_value_ = lower; 
        upper_value_ = higher; 
        is_single_index_ = false;
    }
    void UnlinkIndexedTerm(void) { indexed_term_ = nullptr; }
    virtual bool HasFunction(void);
};

/////////////////////////
//
// STATEMENTS
//
/////////////////////////

class AstBlock : public IAstNode {
public:
    vector<IAstNode*>   block_items_;
    PositionInfo        pos_;
    bool                can_remark_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(can_remark_); }

    AstBlock() { can_remark_ = false; }
    virtual ~AstBlock();
    virtual AstNodeType GetType(void) const { return(ANT_BLOCK); }
    void AddItem(IAstNode *node) { block_items_.push_back(node); }
    void SetRemarkable(void) { can_remark_ = true; }
};

class AstIncDec : public IAstNode {
public:
    Token           operation_;
    IAstExpNode     *left_term_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstIncDec() { if (left_term_ != NULL) delete left_term_; }
    AstIncDec(Token op) : operation_(op), left_term_(NULL) {}
    virtual AstNodeType GetType(void) const { return(ANT_INCDEC); }
    void SetLeftTerm(IAstExpNode *exp) { left_term_ = exp; }
};

class AstSwap : public IAstNode {
public:
    Token           operation_;
    IAstExpNode     *left_term_;
    IAstExpNode     *right_term_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstSwap() { if (left_term_ != NULL) delete left_term_; }
    AstSwap(IAstExpNode *left, IAstExpNode *right) : left_term_(left), right_term_(right) {}
    virtual AstNodeType GetType(void) const { return(ANT_SWAP); }
};

class AstUpdate : public IAstNode {
public:
    Token           operation_;
    IAstExpNode     *left_term_;
    IAstExpNode     *right_term_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstUpdate() { if (left_term_ != NULL) delete left_term_; if (right_term_ != NULL) delete right_term_; }
    AstUpdate(Token op, IAstExpNode *left, IAstExpNode *right) : operation_(op), left_term_(left), right_term_(right) {}
    virtual AstNodeType GetType(void) const { return(ANT_UPDATE); }
};

class AstWhile : public IAstNode {
public:
    IAstExpNode     *expression_;
    AstBlock        *block_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstWhile() { if (expression_ != NULL) delete expression_;  if (block_ != NULL) delete block_; }
    AstWhile() : expression_(NULL), block_(NULL) {}
    virtual AstNodeType GetType(void) const { return(ANT_WHILE); }
    IAstNode *GetTheExpression(void) { return(expression_); }
    IAstNode *GetTheBlock(void) { return(block_); }
    void SetExpression(IAstExpNode *exp) { expression_ = exp; }
    void SetBlock(AstBlock *block) { block_ = block; }
};

class AstIf : public IAstNode {
public:
    vector<IAstExpNode*>    expressions_;
    vector<AstBlock*>       blocks_;
    AstBlock                *default_block_;
    PositionInfo            pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstIf();
    AstIf() : default_block_(NULL) { expressions_.reserve(4); blocks_.reserve(4); }
    virtual AstNodeType GetType(void) const { return(ANT_IF); }
    void AddExpression(IAstExpNode *exp) { expressions_.push_back(exp); }
    void AddBlock(AstBlock *blk) { blocks_.push_back(blk); }
    void SetDefaultBlock(AstBlock *blk) { default_block_ = blk;}
};

class AstFor : public IAstNode {
public:
    VarDeclaration  *index_;
    VarDeclaration  *iterator_;
    IAstExpNode     *set_;
    IAstExpNode     *low_;
    IAstExpNode     *high_;
    IAstExpNode     *step_;
    AstBlock        *block_;
    PositionInfo    pos_;

    // annotations. 
    bool            index_referenced_;      // where index and iterator aleady declared ?
    int64_t         step_value_;            // 0 if not possible to compute at compile time 

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstFor();
    AstFor() : set_(NULL), low_(NULL), high_(NULL), step_(NULL), block_(NULL), index_(NULL), iterator_(NULL), 
                index_referenced_(false), step_value_(1) {}
    virtual AstNodeType GetType(void) const { return(ANT_FOR); }
    void SetIndexVar(VarDeclaration *var) { index_ = var; }
    void SetIteratorVar(VarDeclaration *var) { iterator_ = var; }
    void SetTheSet(IAstExpNode *exp) { set_ = exp; }
    void SetLowBound(IAstExpNode *exp) { low_ = exp; }
    void SetHightBound(IAstExpNode *exp) { high_ = exp; }
    void SetStep(IAstExpNode *exp) { step_ = exp; }
    void SetBlock(AstBlock *block) { block_ = block; }
    void SetStepValue(int64_t value) { step_value_ = value; }
};

class AstSimpleStatement : public IAstNode {
public:
    Token           subtype_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    AstSimpleStatement(Token type) : subtype_(type) {}
    virtual AstNodeType GetType(void) const { return(ANT_SIMPLE); }
};

class AstReturn : public IAstNode {
public:
    IAstExpNode     *retvalue_;
    PositionInfo    pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstReturn() { if (retvalue_ != NULL) delete retvalue_; }
    AstReturn() : retvalue_(NULL) {}
    virtual AstNodeType GetType(void) const { return(ANT_RETURN); }
    void AddRetExp(IAstExpNode *exp) { retvalue_ = exp; }
};

class AstSwitch : public IAstNode {
public:
    IAstExpNode             *switch_value_;
    vector<IAstExpNode*>    case_values_;
    vector<IAstNode*>       statements_;
    vector<int>             statement_top_case_;
    bool                    has_default;
    PositionInfo    pos_;

    // attributes
    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstSwitch();
    AstSwitch() : switch_value_(nullptr), has_default(false) {}
    virtual AstNodeType GetType(void) const { return(ANT_SWITCH); }
    void AddSwitchValue(IAstExpNode *exp) { switch_value_ = exp; }
    void AddCase(IAstExpNode *exp) { case_values_.push_back(exp); }
    void AddStatement(IAstNode *statement) { 
        statements_.push_back(statement); 
        statement_top_case_.push_back(case_values_.size());
    }
    void AddDefaultStatement(IAstNode *statement) { 
        AddStatement(statement);
        has_default = true;
    }
};

class AstTypeSwitch : public IAstNode {
public:
    VarDeclaration          *reference_;
    IAstExpNode             *expression_;
    vector<IAstTypeNode*>   case_types_;
    vector<IAstNode*>       case_statements_;
    PositionInfo            pos_;

    // annotations
    bool                    on_interface_ptr_;
    vector<bool>            uses_reference_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~AstTypeSwitch();
    AstTypeSwitch() : expression_(NULL), reference_(nullptr), on_interface_ptr_(false) {}
    virtual AstNodeType GetType(void) const { return(ANT_TYPESWITCH); }
    void Init(VarDeclaration *ref, IAstExpNode *exp) { reference_ = ref; expression_ = exp; }
    void AddCase(IAstTypeNode *the_type, IAstNode *statement) {
        case_types_.push_back(the_type); 
        case_statements_.push_back(statement); 
    }
    bool SetSwitchOnInterfacePointer(void) { on_interface_ptr_ = true; }
    void SetReferenceUsage(bool value) { uses_reference_.push_back(value); }
};

/////////////////////////
//
// BASE STRUCTURE OF PACKAGE
//
/////////////////////////
class AstIniter : public IAstNode {
public:
    vector<IAstNode*>   elements_;
    PositionInfo        pos_;

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstIniter();
    AstIniter() { elements_.reserve(4); }
    virtual AstNodeType GetType(void) const { return(ANT_INITER); }
    void AddElement(IAstNode *element) { elements_.push_back(element); }
};

class VarDeclaration : public IAstDeclarationNode
{
public:
    string          name_;
    int32_t         flags_;             // annotations
    IAstTypeNode    *type_spec_;
    IAstNode        *initer_;
    PositionInfo    pos_;
    bool            is_public_;

    IAstTypeNode    *weak_type_spec_;   // annotation: weak pointer to a type spec. (for 'for loop' iterators and auto variables).
    VarDeclaration  *weak_iterated_var_; // annotation: null if the var is not a for iterator. 

    virtual bool IsPublic(void) { return(is_public_); }
    virtual void SetPublic(bool value) { is_public_ = value; }

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~VarDeclaration() { if (type_spec_ != nullptr) delete type_spec_; if (initer_ != nullptr) delete initer_; }
    VarDeclaration(const char *name) : 
        name_(name), flags_(0), type_spec_(nullptr), initer_(nullptr), 
        weak_type_spec_(nullptr), is_public_(false), weak_iterated_var_(nullptr) {}
    virtual AstNodeType GetType(void) const { return(ANT_VAR); }
    void SetType(IAstTypeNode *node) { type_spec_ = node; weak_type_spec_ = node; }
    void SetIniter(IAstNode *node) { initer_ = node; }
    void ForceFlags(int32_t flags) { flags_ = flags; }
    void SetFlags(int32_t flags) { flags_ |= flags; }
    void ClearFlags(int32_t flags) { flags_ &= ~flags; }
    bool HasOneOfFlags(int32_t flags) { return((flags_ & flags) != 0); }
    bool HasAllFlags(int32_t flags) { return((flags_ & flags) == flags); }
    void SetTheIteratedVar(VarDeclaration *iterated) { weak_iterated_var_ = iterated; }
    void SetUsageFlags(ExpressionUsage usage);
};

class TypeDeclaration : public IAstDeclarationNode
{
public:
    string          name_;
    IAstTypeNode    *type_spec_;
    PositionInfo    pos_;
    bool            is_public_;

    bool                    is_used_;             // annotations
    ForwardReferenceType    forward_referral_;

    virtual bool IsPublic(void) { return(is_public_); }
    virtual void SetPublic(bool value) { is_public_ = value; }

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~TypeDeclaration() { if (type_spec_ != NULL) delete type_spec_; }
    TypeDeclaration(const char *name) : name_(name), type_spec_(NULL), is_public_(false), is_used_(false), forward_referral_(FRT_NONE) {}
    virtual AstNodeType GetType(void) const { return(ANT_TYPE); }
    void SetType(IAstTypeNode *node) { type_spec_ = node; }
    void SetUsed(void) { is_used_ = true; }
    void SetForwardReferred(ForwardReferenceType mode) { forward_referral_ = mode; }
};

class FuncDeclaration : public IAstDeclarationNode
{
public:
    string          name_;
    bool            is_class_member_;
    bool            is_muting_;
    string          classname_;
    AstFuncType     *function_type_;
    AstBlock        *block_;
    PositionInfo    pos_;
    bool            is_public_;

    bool            is_used_;           // annotations
    bool            type_is_ok_;
    bool            is_virtual_;

    virtual bool IsPublic(void) { return(is_public_); }
    virtual void SetPublic(bool value) { is_public_ = value; }

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    virtual ~FuncDeclaration() { if (function_type_ != NULL) delete function_type_; if (block_ != NULL) delete block_; }
    FuncDeclaration() : function_type_(NULL), block_(NULL), is_public_(false), is_used_(false), type_is_ok_(false), is_muting_(false), is_virtual_(false) {}
    virtual AstNodeType GetType(void) const { return(ANT_FUNC); }
    void SetNames(const char *name1, const char *name2);
    void AddType(AstFuncType *type) { function_type_ = type; }
    void AddBlock(AstBlock *block) { block_ = block; }
    void SetUsed(void) { is_used_ = true; }
    void SetOk(void) { type_is_ok_ = true; }
    void SetMuting(bool is_muting) { is_muting_ = is_muting; }
    void SetVirtual(bool is_virtual) { is_virtual_ = is_virtual; }
};

enum class DependencyUsage {UNUSED, PRIVATE, PUBLIC};   // referred by private or public symbols ?

class AstDependency : public IAstNode {
public:
    string          package_dir_;
    string          package_name_;
    PositionInfo    pos_;

    // annotations
    int             package_index_;
    bool            ambiguous_;          // i.e. another dependency has different path and same local name 
    string          full_package_path_;  // including search path
private:
    DependencyUsage usage_;
public:

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(true); }

    //virtual ~AstDependency();
    AstDependency(const char *path, const char *name);
    virtual AstNodeType GetType(void) const { return(ANT_DEPENDENCY); }
    void SetLocalPackageName(const char *name) { package_name_ = name; }
    void SetUsage(DependencyUsage usage);
    DependencyUsage GetUsage(void) { return(usage_); }
};

class AstFile : public IAstNode {
public:
    //string                          package_name_;
    string                          namespace_;
    vector<AstDependency*>          dependencies_;
    vector<IAstDeclarationNode*>    declarations_;
    vector<RemarkDescriptor*>       remarks_;
    PositionInfo                    pos_;
    NamesList                       private_symbols_;   // gets filled if parsing for reference (privates are not in the tree !!)

    virtual PositionInfo *GetPositionRecord(void) { return(&pos_); }
    virtual bool IsARemarkableNode(void) { return(false); }

    virtual ~AstFile();
    AstFile() {
        dependencies_.reserve(8); 
        declarations_.reserve(16);
    }
    virtual AstNodeType GetType(void) const { return(ANT_FILE); }
    void AddDependency(AstDependency *dep) { dependencies_.push_back(dep); }
    void AddNode(IAstDeclarationNode *node) { declarations_.push_back(node); }
    void SetNamespace(const char *value) { namespace_ = value; }
    void AddPrivateSymbol(const char *value) { private_symbols_.AddName(value); }
};

// solves a type <name> <def> declaration
class ITypedefSolver {
public:
    // CONST is returned when the types differ in constness
    // NONCOPY is returned when the type is not copyable.
    enum TypeMatchResult { OK, KO, CONST, NONCOPY }; 

    //virtual bool            CheckArrayIndicesInTypes(AstArrayType *array) = 0;
    virtual TypeMatchResult AreTypeTreesCompatible(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode) = 0;
};

};

#endif