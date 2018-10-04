#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "Parser.h"

namespace SingNames {

class Compiler {
    Lexer   lexer_;
    Parser  parser_;

    void TestLexer(void);
    void TestParser(void);
public:
    void Run(void);
};


} // namespace

#endif
