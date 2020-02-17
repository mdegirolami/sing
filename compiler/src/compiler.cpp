#include <limits.h>
#include <float.h>
#include "compiler.h"
#include "helpers.h"
#include "ast_nodes_print.h"
#include "FileName.h"

int main(int argc, char *argv[])
{
    SingNames::Compiler compiler;
    compiler.Run(argc, argv);
    return(0);
}

namespace SingNames {

void Compiler::Run(int argc, char *argv[])
{
    if (!options_.ParseArgs(argc, argv)) {
        return;
    }
    switch (options_.GetTestMode()) {
    case 1:
        TestLexer();
        return;
    case 2:
        TestParser();
        return;
    case 3:
        TestChecker();
        return;
    }
    CompileSinglePackage();
}

void Compiler::TestChecker(void)
{
    Package     *pkg = new Package;

    packages_.push_back(pkg);
    pkg->Init(options_.GetSourceName());    // -p improperly used !!
    if (!pkg->Load(PkgStatus::FULL)) {
        PrintPkgErrors(pkg);
    } else if (!checker_.CheckAll(&packages_, &options_, 0, true)) {
        PrintAllPkgErrors();
    }
}

void Compiler::TestParser(void)
{
    Package     *pkg = new Package;

    packages_.push_back(pkg);
    pkg->Init(options_.GetSourceName());    // -p improperly used !!
    if (!pkg->Load(PkgStatus::FULL)) {
        PrintPkgErrors(pkg);
        getchar();
    } else {
        AstNodesPrint   printer;

        FILE *print_dst = fopen(options_.GetOutputFile(), "wt");
        printer.Init(print_dst);
        printer.PrintFile(pkg->root_);
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
        try {
            error = false;
            lexer.Advance();
        }
        catch (ParsingException ex) {
            printf("\n\nERROR !! %s at %d, %d\n", ex.description, ex.row, ex.column);
            lexer.ClearError();
            error = true;
        }
        token = lexer.CurrToken();
        if (token != TOKEN_EOF && !error) {
            printf("\n%d\t%s", token, lexer.CurrTokenVerbatim());
        }
    } while (token != TOKEN_EOF);
    lexer.CloseFile();
}

void Compiler::CompileSinglePackage(void)
{
    Package     *pkg = new Package;

    packages_.push_back(pkg);
    pkg->Init(options_.GetSourceName());    // -p improperly used !!
    if (!pkg->Load(PkgStatus::FULL)) {
        PrintPkgErrors(pkg);
    } else if (!checker_.CheckAll(&packages_, &options_, 0, true)) {
        PrintAllPkgErrors();
    } else {
        string output_name;
        FILE *cppfd = nullptr;
        FILE *hfd = nullptr;
        bool empty_cpp;

        cpp_synthesizer_.Init();
        output_name = options_.GetOutputFile();     // -o improperly used

        FileName::ExtensionSet(&output_name, "cpp");
        cppfd = fopen(output_name.c_str(), "wb");
        if (cppfd == NULL) {
            printf("\ncan't open output file: %s", output_name.c_str());
            return;
        }

        FileName::ExtensionSet(&output_name, "h");
        hfd = fopen(output_name.c_str(), "wb");
        if (hfd == NULL) {
            printf("\ncan't open output file: %s", output_name.c_str());
            fclose(cppfd);
            return;
        }

        cpp_synthesizer_.Synthetize(cppfd, hfd, &packages_, 0, &empty_cpp);
        fclose(cppfd);
        fclose(hfd);
        if (empty_cpp) {
            FileName::ExtensionSet(&output_name, "cpp");
            unlink(output_name.c_str());
        }
    }
}

void Compiler::PrintAllPkgErrors()
{
    int len = packages_.size();
    for (int ii = len-1; ii >= 0; --ii) {
        PrintPkgErrors(packages_[ii]);
    }
}

void Compiler::PrintPkgErrors(Package *pkg)
{
    int         error_idx = 0;
    const char  *error;

    pkg->SortErrors();
    do {
        error = pkg->GetError(error_idx++);
        if (error != NULL) {
            printf("\nERROR !! file %s: %s", pkg->fullpath_.c_str() ,error);
        }
    } while (error != NULL);
}

}