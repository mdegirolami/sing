#include <limits.h>
#include <float.h>
#include "compiler.h"
#include "helpers.h"
#include "test_visitor.h"

void main(void)
{
    SingNames::Compiler compiler;

    compiler.Run();
    getchar();
}

namespace SingNames {

void Compiler::Run(void)
{
    TestParser();
}

void Compiler::TestParser(void)
{
    TestVisitor visitor;
    AstFile *ast;
    FILE    *visitor_dst;
    const char *error;
    int     error_idx;

    if (lexer_.OpenFile("../examples/first/parser_errors_check.sing")) {
        printf("\ncan't open file");
        return;
    }
    parser_.Init(&lexer_);
    ast = parser_.ParseAll();
    if (ast == NULL) {
        error_idx = 0;
        do {
            error = parser_.GetError(error_idx++);
            if (error != NULL) {
                printf("\nERROR !! %s", error);
            }
        } while (error != NULL);
        lexer_.CloseFile();
        return;
    }
    lexer_.CloseFile();

    visitor_dst = fopen("../examples/first/parser_errors_check.txt", "wt");
    visitor.Init(visitor_dst, &lexer_);
    ast->Visit(&visitor);
    fclose(visitor_dst);
}

void Compiler::TestLexer(void)
{
    Token token;
    bool  error;

    if (lexer_.OpenFile("../examples/lexertest/test.stay")) {
        printf("\ncan't open file");
        return;
    }
    do {
        try {
            error = false;
            lexer_.Advance();
        }
        catch (ParsingException ex) {
            printf("\n\nERROR !! %s at %d, %d\n", ex.description, ex.row, ex.column);
            lexer_.ClearError();
            error = true;
        }
        token = lexer_.CurrToken();
        if (token != TOKEN_EOF && !error) {
            printf("\n%d\t%s", token, lexer_.CurrTokenVerbatim());
        }
    } while (token != TOKEN_EOF);
    lexer_.CloseFile();
}

}