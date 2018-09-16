#include <limits.h>
#include <float.h>
#include "compiler.h"
#include "helpers.h"

void main(void)
{
    StayNames::Compiler compiler;

    compiler.Run();
    getchar();
}

namespace StayNames {

void Compiler::Run(void)
{
    if (lexer.OpenFile("../../examples/first/first_program.txt")) {
        printf("\ncan't open file");
        return;
    }
    parser.Init(&lexer);
    try {
        parser.ParseAll();
    } catch(ParsingException ex) {
        printf("\n\nERROR !! %s at %d, %d\n", ex.description, ex.row, ex.column);
        lexer.ClearError();
    }
    lexer.CloseFile();
}

void Compiler::TestLexer(void)
{
    Token token;
    bool  error;

    uint64_t a = 18446744073709551615;
    double b = 1.7976931348623158e308;

    if (lexer.OpenFile("../../examples/lexertest/test.stay")) {
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

}