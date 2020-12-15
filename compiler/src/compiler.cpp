#include <limits.h>
#include <float.h>
#include <assert.h>
#include "compiler.h"
#include "helpers.h"
#include "ast_nodes_print.h"
#include "FileName.h"

int main(int argc, char *argv[])
{
    SingNames::Compiler compiler;
    return(compiler.Run(argc, argv));
}

namespace SingNames {

int Compiler::Run(int argc, char *argv[])
{
    if (!options_.ParseArgs(argc, argv)) {
        return(0);
    }
    pmgr_.init(&options_);
    switch (options_.GetTestMode()) {
    case 1:
        TestLexer();
        return(0);
    case 2:
        TestParser();
        return(0);
    case 3:
        TestChecker();
        return(0);
    }
    return(CompileSinglePackage());
}

void Compiler::TestChecker(void)
{
    int idx = pmgr_.init_pkg(options_.GetSourceName());
    if (!pmgr_.load(idx, PkgStatus::FULL)) {
        PrintPkgErrors(pmgr_.getPkg(idx));
    } else if (!pmgr_.check(idx, true)) {
        PrintAllPkgErrors();
    }
}

void Compiler::TestParser(void)
{
    int idx = pmgr_.init_pkg(options_.GetSourceName());
    if (!pmgr_.load(idx, PkgStatus::FULL)) {
        PrintPkgErrors(pmgr_.getPkg(idx));
    } else {
        AstNodesPrint   printer;

        FILE *print_dst = fopen(options_.GetOutputFile(), "wt");
        printer.Init(print_dst);
        printer.PrintFile(pmgr_.getPkg(idx)->GetRoot());
        fclose(print_dst);
    }
}

void Compiler::TestLexer(void)
{
    Token  token;
    bool   error;
    Lexer  lexer;

    if (lexer.OpenFile(options_.GetSourceName())) {   // -p improperly used !!
        printf("\ncan't open file");
        return;
    }
    do {
        if (!lexer.Advance(&token)) {
            int row, col;
            string mess;
            lexer.GetError(&mess, &row, &col);
            printf("\n\nERROR !! %s at %d, %d\n", mess.c_str(), row, col);
            lexer.ClearError();
        } else if (token != TOKEN_EOF) {
            printf("\n%d\t%s", token, lexer.CurrTokenVerbatim());
        }
    } while (token != TOKEN_EOF);
}

int Compiler::CompileSinglePackage(void)
{
    bool h_only = options_.GenerateHOnly();

    int idx = pmgr_.init_pkg(options_.GetSourceName());
    if (!pmgr_.load(idx, h_only ? PkgStatus::FOR_REFERENCIES : PkgStatus::FULL)) {
        PrintPkgErrors(pmgr_.getPkg(idx));
    } else if (!pmgr_.check(idx, true)) {
        PrintAllPkgErrors();
    } else {
        string output_name;
        FILE *cppfd = nullptr;
        FILE *hfd = nullptr;
        bool empty_cpp;

        cpp_synthesizer_.Init();
        output_name = options_.GetOutputFile();

        if (!h_only) {
            FileName::ExtensionSet(&output_name, "cpp");
            cppfd = fopen(output_name.c_str(), "wb");
            if (cppfd == nullptr) {
                printf("\ncan't open output file: %s", output_name.c_str());
                return(1);
            }
        }

        FileName::ExtensionSet(&output_name, "h");
        hfd = fopen(output_name.c_str(), "wb");
        if (hfd == NULL) {
            printf("\ncan't open output file: %s", output_name.c_str());
            fclose(cppfd);
            return(1);
        }

        cpp_synthesizer_.Synthetize(cppfd, hfd, &pmgr_, &options_, 0, &empty_cpp);
        if (cppfd != nullptr) fclose(cppfd);
        fclose(hfd);

        if (h_only) {
            return(0);
        }

        // dont delete an empty cpp: this would cause ninja to repeat the build !!
        // if (empty_cpp) {
        //     FileName::ExtensionSet(&output_name, "cpp");
        //     unlink(output_name.c_str());
        // }

        FileName::ExtensionSet(&output_name, "map");
        FILE *mfd = fopen(output_name.c_str(), "wb");
        if (mfd == NULL) {
            printf("\ncan't open map file: %s", output_name.c_str());
            return(1);
        }
        cpp_synthesizer_.SynthMapFile(mfd);
        fclose(mfd);

        if (options_.MustCreateDFile()) {
            FileName::ExtensionSet(&output_name, "h");
            FILE *dfd = fopen((output_name + ".d").c_str(), "wb");
            if (dfd == NULL) {
                printf("\ncan't open output file: %s", output_name.c_str());
                return(1);
            }
            cpp_synthesizer_.SynthDFile(dfd, pmgr_.getPkg(idx), output_name.c_str());
            fclose(dfd);
        }
        return(0);
    }
    return(1);
}

void Compiler::PrintAllPkgErrors()
{
    int len = pmgr_.getPkgsNum();
    for (int ii = len-1; ii >= 0; --ii) {
        PrintPkgErrors(pmgr_.getPkg(ii));
    }
}

void Compiler::PrintPkgErrors(const Package *pkg)
{
    int         error_idx = 0;
    const char  *error;
    bool        has_errors = false;

    if (pkg == nullptr) return;
    do {
        error = pkg->GetError(error_idx++);
        if (error != NULL) {
            printf("\nERROR !! file %s:%s", pkg->getFullPath() ,error);
            has_errors = true;
        }
    } while (error != NULL);
    if (has_errors) {
        printf("\n");
    }
}

}