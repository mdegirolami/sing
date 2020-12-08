#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "Parser.h"
#include "ast_checks.h"
#include "Options.h"
#include "cpp_synth.h"
#include "package_manager.h"

namespace SingNames {

class Compiler {
    PackageManager   pmgr_;
    AstChecker       checker_;
    Options          options_;
    CppSynth         cpp_synthesizer_;

    void TestLexer(void);
    void TestParser(void);
    void TestChecker(void);
    int  CompileSinglePackage(void);

    void PrintAllPkgErrors();
    void PrintPkgErrors(const Package *pkg);
public:
    int Run(int argc, char *argv[]);
};

} // namespace

#endif
