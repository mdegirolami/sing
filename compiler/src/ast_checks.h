#ifndef AST_CHECKS_H
#define AST_CHECKS_H

#include <unordered_map>
#include "ast_nodes.h"
#include "numeric_value.h"
#include "symbols_storage.h"
#include "expression_attributes.h"
#include "package.h"

namespace SingNames {

enum TypeSpecCheckMode {
    TSCM_STD,
    TSCM_INITEDVAR,     // allow []
    TSCM_RETVALUE,      // allow void
    TSCM_REFERENCED     // allow class forward ref (if in a class) and interfaces
};

enum class ExpressionUsage {WRITE, READ, NONE, BOTH};

class AstChecker : public ITypedefSolver {
    vector<Package*>        *packages_;
    AstFile                 *root_;         // not owned !!!
    ErrorList               *errors_;       // not owned !!!
    SymbolsStorage          *symbols_;      // not owned !!!
    Options                 *options_;      // not owned !!!
    ErrorList               usage_errors_;
    bool                    check_usage_errors_;

    // info about the currently checked declaration
    int                     current_;
    bool                    current_is_public_;
    int                     loops_nesting;

    // info about the currently checked function block
    ExpressionAttributes    return_fake_variable_;
    bool                    in_function_block_;
    FuncDeclaration         *current_function_;
    AstClassType            *current_class_;
    bool                    this_was_accessed_;
    bool                    in_class_declaration_;

    // tree parser
    void CheckVar(VarDeclaration *declaration);
    void CheckMemberVar(VarDeclaration *declaration);
    void CheckType(TypeDeclaration *declaration);
    void CheckFunc(FuncDeclaration *declaration);
    void CheckMemberFunc(FuncDeclaration *declaration);
    void CheckFuncBody(FuncDeclaration *declaration);

    bool CheckTypeSpecification(IAstNode *type_spec, TypeSpecCheckMode mode);
    bool CheckIniter(IAstTypeNode *type_spec, IAstNode *initer);
    bool CheckArrayIniter(AstArrayType *type_spec, IAstNode *initer);
    void CheckEnum(AstEnumType *declaration);
    void CheckInterface(AstInterfaceType *declaration);
    void CheckClass(AstClassType *declaration);
    void CheckMemberFuncDeclaration(FuncDeclaration *declaration, bool from_interface_declaration);

    void CheckBlock(AstBlock *block, bool open_scope);
    AstNodeType CheckStatement(IAstNode *statement);
    void CheckUpdateStatement(AstUpdate *node);
    void CheckIncDec(AstIncDec *node);
    void CheckWhile(AstWhile *node);
    void CheckIf(AstIf *node);
    void CheckFor(AstFor *node);
    void CheckSwitch(AstSwitch *node);
    void CheckTypeSwitch(AstTypeSwitch *node);
    void CheckSimpleStatement(AstSimpleStatement *node);
    void CheckReturn(AstReturn *node);

    void CheckExpression(IAstExpNode *node, ExpressionAttributes *attr, ExpressionUsage usage);

    void CheckIndices(AstIndexing *node, ExpressionAttributes *attr, ExpressionUsage usage);
    void CheckFunCall(AstFunCall *node, ExpressionAttributes *attr);
    void CheckBinop(AstBinop *node, ExpressionAttributes *attr);
    void CheckUnop(AstUnop *node, ExpressionAttributes *attr);
    void CheckDotOp(AstBinop *node, ExpressionAttributes *attr, ExpressionUsage usage, bool dotop_left);
    void CheckMemberAccess(AstExpressionLeaf *accessed, vector<FuncDeclaration*> *member_functions, vector<VarDeclaration*> *member_vars, ExpressionAttributes *attr, ExpressionUsage usage);
    void CheckDotOpLeftLeaf(AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage);
    void CheckLeaf(AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage);
    void CheckNamedLeaf(IAstDeclarationNode *decl, AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage, bool preceeds_dotop);

    bool VerifyIndexConstness(IAstExpNode *node);
    bool VerifyBinopForIndexConstness(AstBinop *node);
    bool VerifyUnopForIndexConstness(AstUnop *node);
    bool VerifyLeafForIndexConstness(AstExpressionLeaf *node);

    bool IsCompileTimeConstant(IAstExpNode *node);
    bool IsBinopCompileTimeConstant(AstBinop *node);
    bool IsUnopCompileTimeConstant(AstUnop *node);
    bool IsLeafCompileTimeConstant(AstExpressionLeaf *node);

    void CheckNameConflictsInIfFunctions(AstNamedType *typespec,
        vector<VarDeclaration*> *member_vars,
        vector<FuncDeclaration*> *member_functions,
        vector<AstNamedType*> *origins,
        int first_inherited_fun);

    // symbols
    void InsertName(const char *name, IAstDeclarationNode *declaration);
    IAstDeclarationNode *SearchDeclaration(const char *name, IAstNode *location);
    IAstDeclarationNode *ForwardSearchDeclaration(const char *name, IAstNode *location);
    int SearchAndLoadPackage(const char *name, IAstNode *location, const char *not_found_error_string);
    IAstDeclarationNode *SearchExternDeclaration(int package_index, const char *name, bool *is_private);

    bool FlagLocalVariableAsPointed(IAstExpNode *node);
    bool IsGoodForIndex(IAstDeclarationNode *declaration);
    //bool IsGoodForIterator(IAstDeclarationNode *declaration, IAstTypeNode *type);
    bool IsGoodStringIterator(IAstDeclarationNode *declaration);
    bool IsGoodIntegerIterator(IAstDeclarationNode *declaration, Token type);
    bool CanAssign(ExpressionAttributes *dst, ExpressionAttributes *src, IAstNode *err_location);
    bool AreInterfaceAndClass(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode);
    bool NodeIsConcrete(IAstTypeNode *tt);
    bool BlockReturnsExplicitly(AstBlock *block);
    void CheckIfVarReferenceIsLegal(VarDeclaration *var, IAstNode *location);
    void CheckIfFunCallIsLegal(AstFuncType *func, IAstNode *location);
    void SetUsageFlags(VarDeclaration *var, ExpressionUsage usage);
    VarDeclaration *GetIteratedVar(IAstExpNode *node);
    void CheckInnerBlockVarUsage(void);
    void CheckPrivateDeclarationsUsage(void);
    void CheckPrivateVarUsage(VarDeclaration *var, bool is_member = false);
    void CheckPrivateFuncUsage(FuncDeclaration *func);
    void CheckPrivateTypeUsage(TypeDeclaration *tdec);
    void CheckMemberFunctionsDeclarationsPresence(void);
    bool IsArgTypeEligibleForAnIniter(IAstTypeNode *type);
    AstClassType *GetLocalClassTypeDeclaration(const char *classname, bool solve_typedefs);
    FuncDeclaration *SearchFunctionInClass(AstClassType *the_class, const char *name);

    void Error(const char *message, IAstNode *location, bool use_last_location = false);
    void UsageError(const char *message, IAstNode *location, bool use_last_location = false);
public:
    //AstChecker() {}
    bool CheckAll(vector<Package*> *packages, Options *options, int pkg_index, bool fully_parsed);    // returns false on error

    // ITypedefSolver interface
    virtual bool            CheckArrayIndicesInTypes(AstArrayType *array, TypeSpecCheckMode mode);
    virtual TypeMatchResult AreTypeTreesCompatible(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode);
};

}

#endif