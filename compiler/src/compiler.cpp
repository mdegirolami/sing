#include <limits.h>
#include <float.h>
#include <assert.h>
#include <stdio.h>
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
    if (options_.ServerMode()) {
        ServerLoop();
        return(0);     
    }
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
        error = pkg->GetErrorString(error_idx++);
        if (error != NULL) {
            printf("\nERROR !! file %s:%s", pkg->getFullPath() ,error);
            has_errors = true;
        }
    } while (error != NULL);
    if (has_errors) {
        printf("\n");
    }
}

void Compiler::AppendQuotedParameter(string *response, const char *parm)
{
    *response += '"';
    do {
        if (*parm == '\\' || *parm == '"') {
            *response += '\\';
        }
        *response += *parm++;
    } while (*parm != 0);
    *response += '"';
}

void Compiler::ServerLoop(void)
{
    char buffer[1000];
    char *parameters[10];
    bool do_exit = false;

    do {

        // get the command
        if (fgets(buffer, sizeof(buffer), stdin) == nullptr) {
            if (errno) break;
            continue;
        }

        // split the command portions
        char *scan = buffer;
        int num_parms = 0;
        while (*scan != 0 && num_parms < 10) {

            // skip leading blanks
            while (*scan == ' ') ++scan;
            if (*scan == 0 || *scan == '\r' || *scan == '\n') break;
            
            if (*scan == '"') {

                // is a string 
                ++scan;
                parameters[num_parms++] = scan;
                
                // take out of the way the escape sequencies
                char *dst = scan;

                // go to end
                while (*scan != 0 && *scan != '"') {
                    if (*scan == '\\') {
                        ++scan;
                    }
                    *dst++ = *scan++;
                }

                // terminate
                if (*scan != 0) scan++;
                *dst = 0;
            } else {
                parameters[num_parms++] = scan;

                // go to end
                while (*scan != 0 && *scan != ' ' && *scan != '\r' && *scan != '\n') ++scan;

                // terminate
                if (*scan != 0) *scan++ = 0;
            }
        }

        if (num_parms < 1) continue;

        // run the command
        if (strcmp(parameters[0], "src_read") == 0) {
            srv_src_read(num_parms, parameters);
        } else if (strcmp(parameters[0], "src_change") == 0) {
            srv_src_change(num_parms, parameters);
        } else if (strcmp(parameters[0], "src_created") == 0) {
            srv_src_created(num_parms, parameters);
        } else if (strcmp(parameters[0], "src_deleted") == 0) {
            srv_src_deleted(num_parms, parameters);
        } else if (strcmp(parameters[0], "src_renamed") == 0) {
            srv_src_renamed(num_parms, parameters);
        } else if (strcmp(parameters[0], "get_errors") == 0) {
            srv_get_errors(num_parms, parameters);
        } else if (strcmp(parameters[0], "completion_items") == 0) {
            srv_completion_items(num_parms, parameters);
        } else if (strcmp(parameters[0], "signature") == 0) {
            srv_signature(num_parms, parameters);
        } else if (strcmp(parameters[0], "def_position") == 0) {
            srv_def_position(num_parms, parameters);
        } else if (strcmp(parameters[0], "exit") == 0) {
            do_exit = true;
        }
    } while (!do_exit);
}

void Compiler::srv_src_read(int num_parms, char *parameters[])
{
    if (num_parms < 2) return;
    int idx = pmgr_.init_pkg(parameters[1], true);
    pmgr_.load(idx, PkgStatus::LOADED);
}

inline int hex2char(int charpoint)
{
    if (charpoint >= '0' && charpoint <= '9') {
        return(charpoint - '0');
    } else if (charpoint >= 'a' && charpoint <= 'f') {
        return(charpoint - 'a' + 10);
    } else if (charpoint >= 'A' && charpoint <= 'F') {
        return(charpoint - 'A' + 10);
    }
    return(0);
}

void Compiler::srv_src_change (int num_parms, char *parameters[])
{
    char newchars[512];

    // some checks
    if (num_parms < 5) return;
    if ((strlen(parameters[4]) & 1) != 0) return;

    // convert hex digits to bytes
    char *dst = newchars;
    for (const char *scan = parameters[4]; *scan; scan += 2) {
        *dst++ = (hex2char(scan[0]) << 4) + hex2char(scan[1]);
    }
    *dst++ = 0;

    // get the index and make sure the file is loaded
    int idx = pmgr_.init_pkg(parameters[1]);
    pmgr_.load(idx, PkgStatus::LOADED);

    // patch it !!
    int offset = atoi(parameters[2]);
    int length = atoi(parameters[3]);
    pmgr_.applyPatch(idx, offset, offset + length, newchars);
}

void Compiler::srv_src_created(int num_parms, char *parameters[])
{
    srv_src_read(num_parms, parameters);
}

void Compiler::srv_src_deleted(int num_parms, char *parameters[])
{
    if (num_parms < 2) return;
    pmgr_.on_deletion(parameters[1]);
}

void Compiler::srv_src_renamed(int num_parms, char *parameters[])
{
    // must reload with another id: old links don't apply
    if (num_parms < 3) return;
    srv_src_deleted(num_parms, parameters);
    parameters[1] = parameters[2];
    srv_src_read(num_parms, parameters);
}

void Compiler::srv_get_errors (int num_parms, char *parameters[])
{
    if (num_parms < 2) return;
    int idx = pmgr_.init_pkg(parameters[1]);
    pmgr_.load(idx, PkgStatus::FULL);
    pmgr_.check(idx, true);
    const Package *pkg = pmgr_.getPkg(idx);
    if (pkg == nullptr) return;

    int         error_idx = 0;
    const char  *error;
    int         row, col, endrow, endcol;

    do {
        error = pkg->GetError(error_idx++, &row, &col, &endrow, &endcol);
        if (error != nullptr) {
            string response = "set_error ";
            AppendQuotedParameter(&response, parameters[1]);
            AppendQuotedParameter(&response, error);
            printf("%s %d %d %d %d\r\n", response.c_str(), row, col, endrow, endcol);
        }
    } while (error != nullptr);
    printf("set_errors_done \"%s\"\r\n", parameters[1]);
}

// >> completion_items <file>,<line>,<col>
// << set_completion_item <file>,<name>
// << set_completions_done <file>

void Compiler::srv_completion_items(int num_parms, char *parameters[])
{
    if (num_parms < 4) return;
    string response = "set_completion_item ";
    AppendQuotedParameter(&response, parameters[1]);
    printf("%s one\r\n", response.c_str());
    printf("%s two\r\n", response.c_str());
    printf("%s three\r\n", response.c_str());
    response = "set_completions_done ";
    AppendQuotedParameter(&response, parameters[1]);
    printf("%s\r\n", response.c_str());
}

// >> signature <file>,<line>,<col>
// << set_signature <file>,<signature>,<parameter>

void Compiler::srv_signature(int num_parms, char *parameters[])
{
    if (num_parms < 4) return;
    string response = "set_signature ";
    AppendQuotedParameter(&response, parameters[1]);
    printf("%s \"filter(kk i32, k2 i32) void\" 1\r\n", response.c_str());
}

// >> def_position <file>,<row>,<col>
// << definition_of <file>,<line>,<col>

void Compiler::srv_def_position(int num_parms, char *parameters[])
{
    if (num_parms < 4) return;
    string response = "definition_of ";
    AppendQuotedParameter(&response, parameters[1]);
    printf("%s %d, %d\r\n", response.c_str(), atoi(parameters[2])/2, atoi(parameters[3])/2);
}

}

/*

function string_as_unicode_escape(input) {
    function pad_four(input) {
        var l = input.length;
        if (l == 0) return '0000';
        if (l == 1) return '000' + input;
        if (l == 2) return '00' + input;
        if (l == 3) return '0' + input;
        return input;
    }
    var output = '';
    for (var i = 0, l = input.length; i < l; i++)
        output += '\\u' + pad_four(input.charCodeAt(i).toString(16));
    return output;
}

*/