#ifndef CPP_SYNTH_H
#define CPP_SYNTH_H

#include <stdio.h>
#include "ast_nodes.h"

namespace SingNames {

class CppSynth {
    Lexer   *lexer_;
    FILE    *file_;
    int     indent_;
    int     id_sequence_;

    void SynthVar(VarDeclaration *declaration);
    void SynthConst(ConstDeclaration *declaration);
    void SynthType(TypeDeclaration *declaration);
    void SynthFunc(FuncDeclaration *declaration);

    void SynthTypeSpecification(string *dst, IAstNode *type_spec);  // init dst with the var/const/type name
    void SynthIniter(string *dst, IAstNode *initer);                // appends to dst
    void SynthIniterCore(string *dst, IAstNode *initer);

    void SynthBlock(AstBlock *block, bool write_closing_bracket = true);    // assumes { has been written !
    void SynthAssignment(AstAssignment *node);
    void SynthUpdateStatement(AstUpdate *node);
    void SynthIncDec(AstIncDec *node);
    void SynthWhile(AstWhile *node);
    void SynthIf(AstIf *node);
    void SynthFor(AstFor *node);
    void SynthSimpleStatement(AstSimpleStatement *node);
    void SynthReturn(AstReturn *node);

    void SynthExpression(string *dst, IAstNode *node);
    void SynthIndices(string *dst, AstIndexing *node);
    void SynthFunCall(string *dst, AstFunCall *node);
    void SynthBinop(string *dst, AstBinop *node);
    void SynthUnop(string *dst, AstUnop *node);
    void SynthLeaf(string *dst, AstExpressionLeaf *node);

    void Write(string *text, bool add_semicolon = true);
    void AddNewLine(string *dst);
    void CreateUniqueId(string *id);
    const char *GetBaseTypeName(Token token);
public:
    CppSynth(Lexer *lexer) : lexer_(lexer) {}
    void Synthetize(FILE *fd, AstFile *root);
};

}

#endif
