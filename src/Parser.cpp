#include "Parser.h"
#include "helpers.h"

namespace StayNames {

Parser::Parser()
{
}

Parser::~Parser()
{
}

void Parser::Init(Lexer *lexer)
{
    m_lexer = lexer;
}

void Parser::ParseAll(void)
{
    bool done = false;

    if (Advance() != TOKEN_PACKAGE) {
        Error("Expected a Package declaration");
    }
    if (Advance() != TOKEN_NAME) {
        Error("Expected the package name");
    } else {
        // AstFile *file = new AstFile(m_lexer->CurrTokenString());
    }
    Advance();
    while (m_token == TOKEN_REQUIRES) {
        ParseDependency();
    }
    while (m_token != TOKEN_EOF) {
        ParseDeclaration();
    }
}

void Parser::ParseDependency(void)
{
    if (Advance() != TOKEN_STRING) {
        Error("expecting the required package path");
    } 
    //m_builder->AddDependency(m_lexer->CurrTokenString());
    if (Advance() == TOKEN_COMMA) {
        if (Advance() != TOKEN_NAME) {
            Error("expecting the required package local name");
        }
        //m_builder->SetPackageName(m_lexer->CurrTokenString());
        Advance();
    } else {
        //m_builder->SetDefaultPackageName();
    }
}

void Parser::ParseDeclaration(void)
{
    if (m_token == TOKEN_PUBLIC) {
        //m_builder
        Advance();
    }
    switch (m_token) {
    case TOKEN_TYPE:
        Error("type declaration not yet implemented");
        break;
    case TOKEN_VAR:
        ParseVar();
        break;
    case TOKEN_CONST:
        Error("const declaration not yet implemented");
        break;
    case TOKEN_FUNC:
        ParseFunctionDeclaration();
        break;
    default:
        Error("Expected a declaration: type, var, const, func, struct, class, interface, enum");
    }
}

void Parser::ParseVar(void)
{
    if (Advance() != TOKEN_NAME) {
        Error("expecting the variable name");
    }
    Advance();
    if (m_token == TOKEN_VOLATILE) {
        // builder..
        Advance();
    }
    ParseTypeSpecification();
    if (m_token == TOKEN_EQUAL) {
        Advance();
        ParseIniter();
    }
}

/*
type_specification ::= base_type | <type_name> | <pkg_name>.<type_name> |
                        map '(' type_specification ')' type_specification |
                        {‘[‘ ([const_expression]) ‘]’} type_specification |
                        matrix {‘[‘ ([ expression[..expression] ]) ‘]’} type_specification |
                        [const] [weak] ‘*’ type_specification |
                        function_type
*/
void Parser::ParseTypeSpecification(void)
{
    switch (m_token){
    case TOKEN_INT8:
    case TOKEN_INT16:
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT8:
    case TOKEN_UINT16:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
    case TOKEN_STRING:
    case TOKEN_RUNE:
    case TOKEN_BOOL:
    case TOKEN_SIZE_T:
    case TOKEN_ERRORCODE:
    case TOKEN_VOID:
        // builder
        Advance();
        break;
    case TOKEN_NAME:
        ParseFullName();
        break;
    case TOKEN_MAP:
        Error("Maps not yet supported");
        break;
    case TOKEN_SQUARE_OPEN:
        ParseIndices(/*type declare = */true);
        break;
    case TOKEN_MATRIX:
        Error("Matrices not yet supported");
        break;
    case TOKEN_CONST:
    case TOKEN_WEAK:
    case TOKEN_MPY:
        Error("Pointers not yet supported");
        break;
    case TOKEN_ROUND_OPEN:
        Error("Function pointers not yet supported");
        break;
    default:
        Error("Invalid type declaration");
        break;
    }
}

void Parser::ParseFullName(void)
{
    if (m_token != TOKEN_NAME) {
        Error("Name expected");
    }
    string first_name_part = m_lexer->CurrTokenString();
    if (Advance() == TOKEN_DOT) {
        if (Advance() == TOKEN_NAME) {
            string second_name_part = m_lexer->CurrTokenString();
            // builder..
            Advance();
        } else {
            Error("Expecting the second part of a qualifyed name");
        }
    }
}

void Parser::ParseIndices(bool type_declare)
{
    Error("Arrays not yet supported");
}

void Parser::ParseIniter(void)
{
    Error("Initer not yet supported");
}

//function_type ::= [pure] argsdef type_specification
void Parser::ParseFunctionType(void)
{
    if (m_token == TOKEN_PURE) {
        Advance();
    }
    ParseArgsDef();
    ParseTypeSpecification();
}

// argsdef ::=  ‘(’ ( single_argdef ) [',' ...] ‘)’ | '(' … ')'
void Parser::ParseArgsDef(void)
{
    if (m_token != TOKEN_ROUND_OPEN) {
        Error("Expected (");
    }
    if (Advance() != TOKEN_ROUND_CLOSE) {
        do {
            if (m_token == TOKEN_ETC) {
                //builder
                Advance();  // to )
                break;      // because we dont' allow additional arguments
            } else {
                ParseSingleArgDef();
            }
        } while (m_token == TOKEN_COMMA);
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expected )");
        }
    }
    Advance();
}

//single_argdef :: = [arg_direction] <arg_name> type_specification[‘ = ’ initer]
// arg_direction :: = out | io | in
void Parser::ParseSingleArgDef(void)
{
    switch (m_token) {
    case TOKEN_IN:
    case TOKEN_OUT:
    case TOKEN_IO:
        // builder
        Advance();
        break;
    }
    if (m_token != TOKEN_NAME) {
        Error("expecting the argument name");
    }
    Advance();
    ParseTypeSpecification();
    if (m_token == TOKEN_EQUAL) {
        Advance();
        ParseIniter();
    }
}

//func_definition ::= func func_fullname function_type block
void Parser::ParseFunctionDeclaration(void)
{
    Advance();
    ParseFullName();
    ParseFunctionType();
    ParseBlock();
}

//block :: = ‘{ ‘{ block_item } ‘ }’
//block_item :: = var_decl | const_decl | statement | block
/* statements:
( left_term ) ‘=’ ( expression ) |
left_term update_operator expression |
left_term ++ | left_term -- |
functioncall |
while ‘(‘ expression ‘)’ block |
if ‘(‘ expression ‘)’ block {[ else if ‘(‘ expression ‘)’ block ]} [else block] |
for '(' <name> in <expression> ':' <expression> [step <expression>]')' |
for '(' [<name>','] <name> in <name> ')' |
break |
continue |
return expression |
error <errcode_name> |
cleanup block |
run functioncall |
type_switch |
bind '{' <literal_stuff> '}'
throw expression
try block { catch_clause block }
*/
void Parser::ParseBlock(void)
{
    if (m_token != TOKEN_CURLY_OPEN) {
        Error("Expecting the function body starting with {");
    }
    Advance();
    while (m_token != TOKEN_CURLY_CLOSE) {
        switch (m_token) {
        case TOKEN_VAR:
            ParseVar();
            break;
        case TOKEN_CONST:
            Error("const not yet supported");
            break;
        case TOKEN_CURLY_OPEN:
            ParseBlock();
            break;
        default:
            ParseLeftTermStatement();
            break;
            //Error("In a funcion block you can only place: statements, vars/const declarations, blocks");
        }
    }
    Advance();
}

//(left_term)‘ = ’(expression) |
//left_term update_operator expression |
//left_term++ | left_term-- |
//functioncall |    ====>>> i.e. a left term
void Parser::ParseLeftTermStatement(void)
{
    bool done;
    int num_assignee = 0;

    do {
        ParseLeftTerm();
        ++num_assignee;
        if (m_token == TOKEN_COMMA) {
            Advance();
        } else {
            done = true;
        }
    } while (!done);
    switch (m_token) {
    case TOKEN_ASSIGN:
        done = false;
        Advance();
        do {
            ParseExpression();
            --num_assignee;
            if (num_assignee > 0 && m_token != TOKEN_COMMA) {
                Error("There are more variables than values in assignment");
            }
        } while (num_assignee > 0);
        break;
    case TOKEN_UPD_PLUS:
    case TOKEN_UPD_MINUS:
    case TOKEN_UPD_MPY:
    case TOKEN_UPD_DIVIDE:
    case TOKEN_UPD_POWER:
    case TOKEN_UPD_MOD:
    case TOKEN_UPD_SHR:
    case TOKEN_UPD_SHL:
    case TOKEN_UPD_AND:
    case TOKEN_UPD_OR:
    case TOKEN_UPD_LOGICAL_AND:
    case TOKEN_UPD_LOGICAL_OR:
        Advance();
        if (num_assignee > 1) {
            Error("Update assignments must involve a single assignee");
        }
        ParseExpression();
        break;
    case TOKEN_INC:
    case TOKEN_DEC:
        Advance();
        break;
    default:
        break;      // let's assume it is a function call.
    }
}

/*
expression :: = null | false | true | err_ok | err_bounds |
                <Numeral> | <LiteralString> | <LiteralComplex> |
                left_term | sizeof '(' left_term ')' | '(' base_type ')' expression |
                dimof '(' left_term ')' |
                expression binop expression | unop expression | ‘(’ expression ‘)’

binop ::=  ‘+’ | ‘-’ | ‘*’ | ‘/’ | ‘^’ | ‘%’ | 
	  ‘&’ | ‘|’ | ‘>>’ | ‘<<’ |
	  ‘<’ | ‘<=’ | ‘>’ | ‘>=’ | ‘==’ | ‘!=’ | 
	  xor | && | || 

unop ::= ‘-’ | ‘!’ | ‘~’ | '&' | '*'
*/
void Parser::ParseExpression(void)
{
    switch (m_token) {
    case TOKEN_NULL:
    case TOKEN_FALSE:
    case TOKEN_TRUE:
    case TOKEN_LITERAL_STRING:
    case TOKEN_LITERAL_UINT:
    case TOKEN_LITERAL_FLOAT:
    case TOKEN_LITERAL_IMG:
        Advance();
        break;
    case TOKEN_SIZEOF:
    case TOKEN_DIMOF:
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting (");
        }
        Advance();
        ParseLeftTerm();
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting )");
        }
        Advance();
        break;
    case TOKEN_ROUND_OPEN:
        switch (Advance()) {
        case TOKEN_INT8:
        case TOKEN_INT16:
        case TOKEN_INT32:
        case TOKEN_INT64:
        case TOKEN_UINT8:
        case TOKEN_UINT16:
        case TOKEN_UINT32:
        case TOKEN_UINT64:
        case TOKEN_FLOAT32:
        case TOKEN_FLOAT64:
        case TOKEN_COMPLEX64:
        case TOKEN_COMPLEX128:
        case TOKEN_STRING:
        case TOKEN_RUNE:
        case TOKEN_BOOL:
        case TOKEN_SIZE_T:
            if (Advance() != TOKEN_ROUND_CLOSE) {
                Error("Expecting )");
            }
            Advance();
            ParseExpression();
            break;
        default:
            ParseExpression();
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting )");
            }
            Advance();
            break;
        }
        break;
    case TOKEN_MINUS:
    case TOKEN_PLUS:
    case TOKEN_MPY:
    case TOKEN_AND:
    case TOKEN_NOT:
    case TOKEN_LOGICAL_NOT:
        Advance();
        ParseExpression();
        break;
    default:
        ParseLeftTerm("Expecting an expression");
        break;
    }

    // connected by a binop to a second expression ?
    switch (m_token) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_POWER:
    case TOKEN_MOD:
    case TOKEN_AND:
    case TOKEN_OR:
    case TOKEN_XOR:
    case TOKEN_SHL:
    case TOKEN_SHR:
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_LTE:
    case TOKEN_GTE:
    case TOKEN_EQUAL:
    case TOKEN_DIFFERENT:
    case TOKEN_LOGICAL_AND:
    case TOKEN_LOGICAL_OR:
        Advance();
        ParseExpression();
    }
}

//left_term ::= <var_name> | prefixexp ‘[’ index_or_rage ‘]’ | prefixexp ‘.’ <name> |
//              functioncall | ‘(’ prefixexp ‘)’ | '*'left_term
void Parser::ParseLeftTerm(const char *errmess)
{
    switch (m_token) {
    case TOKEN_MPY:
        Advance();
        ParseLeftTerm();
        break;
    case TOKEN_ROUND_OPEN:
        Advance();
        ParseLeftTerm();
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expcting )");
        }
        break;
    case TOKEN_NAME:
        {
            ParseFullName();
            bool done = false;
            while (!done) {
                switch (m_token) {
                case TOKEN_SQUARE_OPEN:
                    ParseIndices();
                    break;
                case TOKEN_DOT:
                    if (Advance() != TOKEN_NAME) {
                        Error("Expecting field name");
                    }
                    Advance();
                    break;
                case TOKEN_ROUND_OPEN:
                    ParseArguments();
                    break;
                default:
                    done = true;
                    break;
                }
            }
        }
        break;
    default:
        Error(errmess != NULL ? errmess : "Expecting a left term (an assignable expression)");
        break;
    }
}

void Parser::ParseIndices(void)
{
    Error("Indices not yet supported");
}

void Parser::ParseArguments(void)
{
    Error("Function calls not yet supported");
}

Token Parser::Advance(void) 
{ 
    do {
        m_token = m_lexer->Advance();
    } while (m_token == TOKEN_COMMENT || m_token == TOKEN_INLINE_COMMENT);
    return(m_token);
}

void Parser::Error(const char *message)
{
    throw(ParsingException(1000, m_lexer->CurrTokenLine(), m_lexer->CurrTokenColumn() + 1, message));
}

}