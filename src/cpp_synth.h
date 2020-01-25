#ifndef CPP_SYNTH_H
#define CPP_SYNTH_H

#include <stdio.h>
#include "ast_nodes.h"
#include "package.h"
#include "cpp_formatter.h"

namespace SingNames {

class CppSynth {
    FILE                *cppfd_;
    FILE                *hfd_;
    FILE                *file_;         // output for the current declaration
    vector<Package*>    *packages_;
    AstFile             *root_;
    int                 indent_;
    int                 split_level_;   // note it is an int so that it can be decremented a lot !! (never overflows)
    int                 exp_level;      // to see if synth_expression is recursing
    Token               return_type_;
    CppFormatter        formatter_;

    // options
    bool    newline_before_function_bracket_;

    enum ParmPassingMethod { PPM_VALUE, PPM_POINTER, PPM_CONSTREF, PPM_REF };
    enum TypeSynthMode { TSS_STD, TSS_VARDECL, TSS_FUNC_DECL }; // 

    void SynthVar(VarDeclaration *declaration);
    void SynthType(TypeDeclaration *declaration);
    void SynthFunc(FuncDeclaration *declaration);

    void SynthTypeSpecification(string *dst, IAstNode *type_spec, bool root_of_fun_parm = false);   // init dst with the var/const/type name
    void SynthFuncTypeSpecification(string *dst, AstFuncType *type_spec, bool prototype);           // init dst with the func name
    void SynthArrayTypeSpecification(string *dst, AstArrayType *type_spec, bool root_of_fun_parm);
    void SynthIniter(string *dst, IAstTypeNode *type_spec, IAstNode *initer);                       // appends to dst
    void SynthIniterCore(string *dst, IAstTypeNode *type_spec, IAstNode *initer);
    void SynthZeroIniter(string *dst, IAstTypeNode *type_spec);

    void SynthBlock(AstBlock *block, bool write_closing_bracket = true);    // assumes { has been written !
    void SynthUpdateStatement(AstUpdate *node);
    void SynthPowerUpdateOperator(string *dst, AstUpdate *node);
    void SynthIncDec(AstIncDec *node);
    void SynthWhile(AstWhile *node);
    void SynthIf(AstIf *node);

    void SynthFor(AstFor *node);
    void SynthForEachOnDyna(AstFor *node);
    void SynthForIntRange(AstFor *node);
    void SynthExpressionAndCastToInt(string *dst, IAstExpNode *node, bool use_int64);

    void SynthSimpleStatement(AstSimpleStatement *node);
    void SynthReturn(AstReturn *node);

    // includes a final conversion if needed/possible (you may need it because of constant expressions)
    int SynthFullExpression(Token target_type, string *dst, IAstExpNode *node);

    int SynthExpression(string *dst, IAstExpNode *node);
    int SynthIndices(string *dst, AstIndexing *node);
    int SynthFunCall(string *dst, AstFunCall *node);
    int SynthBinop(string *dst, AstBinop *node);
    int SynthUnop(string *dst, AstUnop *node);
    int SynthLeaf(string *dst, AstExpressionLeaf *node);

    void SynthComplex64(string *dst, AstExpressionLeaf *node);
    void SynthComplex128(string *dst, AstExpressionLeaf *node);

    int SynthDotOperator(string *dst, AstBinop *node);
    int SynthPowerOperator(string *dst, AstBinop *node);
    int SynthMathOperator(string *dst, AstBinop *node);
    int SynthRelationalOperator(string *dst, AstBinop *node);
    int SynthLogicalOperator(string *dst, AstBinop *node);
    int SynthCastToScalar(string *dst, AstUnop *node, int priority);
    int SynthCastToComplex(string *dst, AstUnop *node, int priority);
    int SynthCastToString(string *dst, AstUnop *node);

    int  WriteHeaders(DependencyUsage usage);
    int  WriteNamespaceOpening(void);
    void WriteNamespaceClosing(int num_levels);
    int  WriteTypeDefinitions(bool public_defs);
    void WritePrototypes(bool public_defs);
    void WriteExternalDeclarations(void);
    int  WriteVariablesDefinitions(void);
    int  WriteFunctions(void);

    void ProcessStringSumOperand(string *format, string *parms, IAstExpNode *node);
    void Write(string *text, bool add_semicolon = true);
    void EmptyLine();
    const char *GetBaseTypeName(Token token);
    int  GetBinopCppPriority(Token token);
    int  GetUnopCppPriority(Token token);
    bool VarNeedsDereference(VarDeclaration *var);
    ParmPassingMethod GetParameterPassingMethod(IAstTypeNode *type_spec, bool input_parm);
    //IAstTypeNode *GetElementType(IAstTypeNode *type_spec);
    void PrependWithSeparator(string *dst, const char *src);
    int AddCast(string *dst, int priority, const char *cast_type);
    void CutDecimalPortionAndSuffix(string *dst);
    void CutSuffix(string *dst);
    int CastIfNeededTo(Token target, Token src_type, string *dst, int priority, bool for_power_op);
    void CastForRelational(Token left_type, Token right_type, string *left, string *right, int *priority_left, int *priority_right);
    int PromoteToInt32(Token target, string *dst, int priority);
    int Protect(string *dst, int priority, int next_priority, bool is_right_term = false);
    bool IsPOD(IAstTypeNode *node);
    Token GetBaseType(IAstTypeNode *node);
    int GetRealPartOfIntegerLiteral(string *dst, AstExpressionLeaf *node, int nbits);
    void GetRealPartOfUnsignedLiteral(string *dst, AstExpressionLeaf *node);
    int GetRealPartOfFloatLiteral(string *dst, AstExpressionLeaf *node);
    void GetImgPartOfLiteral(string *dst, const char *src, bool is_double, bool is_negated);
    void GetFullExternName(string *full, int pkg_index, const char *local_name);
    bool IsLiteralString(IAstExpNode *node);
    void AddSplitMarker(string *dst);
    void SetFormatterRemarks(IAstNode *node);
public:
    void Init() { newline_before_function_bracket_ = true; }
    void Synthetize(FILE *cppfd, FILE *hfd, vector<Package*> *packages, int pkg_index, bool *empty_cpp);
};

}

#endif
