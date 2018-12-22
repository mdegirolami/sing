#ifndef AST_CHECKS_H
#define AST_CHECKS_H

#include <unordered_map>
#include "ast_nodes.h"
#include "numeric_value.h"
#include "symbols_storage.h"
#include "expression_attributes.h"

namespace SingNames {

enum TypeSpecCheckMode {TSCM_STD, TSCM_RETVALUE, TSCM_FUNC_BODY};

class AstChecker : public ITypedefSolver {
    AstFile         *root_;
    int             current_;
    NamesList       errors_;
    SymbolsStorage  symbols_;
    int             loops_nesting;
    ExpressionAttributes return_fake_variable_;

    // tree parser
    void CheckVar(VarDeclaration *declaration);
    void CheckConst(ConstDeclaration *declaration);
    void CheckType(TypeDeclaration *declaration);
    void CheckFunc(FuncDeclaration *declaration);

    bool CheckTypeSpecification(IAstNode *type_spec, TypeSpecCheckMode mode);
    bool CheckIniter(IAstTypeNode *type_spec, IAstNode *initer);
    bool CheckArrayIniter(AstArrayOrMatrixType *type_spec, int array_dimension, IAstNode *initer);

    void CheckBlock(AstBlock *block);
    void CheckAssignment(AstAssignment *node);
    void CheckUpdateStatement(AstUpdate *node);
    void CheckIncDec(AstIncDec *node);
    void CheckWhile(AstWhile *node);
    void CheckIf(AstIf *node);
    void CheckFor(AstFor *node);
    void CheckSimpleStatement(AstSimpleStatement *node);
    void CheckReturn(AstReturn *node);

    void CheckExpression(IAstExpNode *node, ExpressionAttributes *attr);
    
    void CheckIndices(AstIndexing *node, ExpressionAttributes *attr);
    void CheckFunCall(AstFunCall *node, ExpressionAttributes *attr);
    void CheckBinop(AstBinop *node, ExpressionAttributes *attr);
    void CheckUnop(AstUnop *node, ExpressionAttributes *attr);
    void CheckLeaf(AstExpressionLeaf *node, ExpressionAttributes *attr);

    // symbols
    void InsertName(const char *name, IAstDeclarationNode *declaration);
    IAstDeclarationNode *FindDeclaration(const char *name, bool search_forward = true);
    bool FindNamespace(const char *name);               // true if found

    bool IsALocalVariable(IAstExpNode *node);
    bool CanAssign(ExpressionAttributes *dst, ExpressionAttributes *src);
    bool CompareTypeTrees(IAstTypeNode *dst_tree, IAstTypeNode *src_tree);
    IAstTypeNode *SolveTypedefs(IAstTypeNode *begin);
    bool CompareTypesForAssignment(IAstTypeNode *dst, IAstTypeNode *src);

    bool FailIfTypeDeclaration(IAstNode *node);         // true if fails
    bool FailIfNotTypeDeclaration(IAstNode *node);      // true if fails
    void DupSymbolError(IAstNode *old_declaration, IAstNode *new_declaration);
    void Error(const char *message);
public:
    //AstChecker() {}
    bool CheckAll(AstFile *root);    // returns false on error

    // ITypedefSolver interface
    IAstTypeNode *TypeFromTypeName(const char *name);
    virtual bool CheckArrayIndices(AstArrayOrMatrixType *array) = 0;
};

}

#endif