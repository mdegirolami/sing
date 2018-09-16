#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "Parser.h"

namespace StayNames {

class Compiler {
    Lexer   lexer;
    Parser  parser;
public:
    void Run(void);
    void TestLexer(void);
};


} // namespace

#endif
