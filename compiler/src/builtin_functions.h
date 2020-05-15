#ifndef BUILTIN_FUNCTIONS_H
#define BUILTIN_FUNCTIONS_H

#include "expression_attributes.h"
#include "ast_nodes.h"

namespace SingNames {
    AstFuncType *GetFuncSignature(bool *ismuting, BInSynthMode *mode, const char *name, const ExpressionAttributes *attr);
}; // namespace

#endif
