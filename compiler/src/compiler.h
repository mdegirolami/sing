#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "Parser.h"
#include "ast_checks.h"
#include "options.h"
#include "cpp_synth.h"
#include "package_manager.h"

namespace SingNames {

class Compiler {
    PackageManager   pmgr_;
    AstChecker       checker_;
    Options          options_;
    CppSynth         cpp_synthesizer_;
    FILE             *server_log_;

    void TestLexer(void);
    void TestParser(void);
    void TestChecker(void);
    int  CompileSinglePackage(void);

    void PrintAllPkgErrors();
    void PrintPkgErrors(const Package *pkg);

    // server stuff
    void ServerLoop(bool log_server);
    void AppendQuotedParameter(string *response, const char *parm);
    void ServerResponse(const char *format, ...);
    void srv_src_read(int num_parms, char *parameters[]);
    void srv_src_change (int num_parms, char *parameters[]);
    void srv_src_insert (int num_parms, char *parameters[]);
    void srv_src_created(int num_parms, char *parameters[]);
    void srv_src_deleted(int num_parms, char *parameters[]);
    void srv_src_renamed(int num_parms, char *parameters[]);
    void srv_get_errors (int num_parms, char *parameters[]);
    void srv_completion_items(int num_parms, char *parameters[]);
    void srv_signature(int num_parms, char *parameters[]);
    void srv_def_position(int num_parms, char *parameters[]);
    void srv_get_symbols(int num_parms, char *parameters[]);

public:
    int Run(int argc, char *argv[], bool log_server);
};

} // namespace

#endif
