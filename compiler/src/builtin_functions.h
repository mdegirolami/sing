#ifndef BUILTIN_FUNCTIONS_H
#define BUILTIN_FUNCTIONS_H

#include "expression_attributes.h"
#include "NamesList.h"

namespace SingNames {
    
// implementation mode for a built-in function
// sing = sing::name(T);
// cast = (T)name(T);       // cast if not double
// plain = name(T);
// memeber = T.member()
enum class BInSynthMode { sing, cast, plain, member, std };

const char *GetFuncSignature(const char *name, const ExpressionAttributes *attr);
AstFuncType *GetFuncTypeFromSignature(const char *signature, const ExpressionAttributes *attr);
BInSynthMode GetBuiltinSynthMode(const char *signature);
char GetBuiltinArgType(const char *signature, int idx); 
void GetBuiltinNames(NamesList *names, const ExpressionAttributes *attr);

}; // namespace

#endif
