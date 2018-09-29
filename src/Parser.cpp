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

AstFile *Parser::ParseAll(void)
{
    bool    done = false;
    AstFile *file = NULL;

    if (Advance() != TOKEN_PACKAGE) {
        Error("Expected a Package declaration");
    }
    if (Advance() != TOKEN_NAME) {
        Error("Expected the package name");
    } else {
        file = new AstFile(m_lexer->CurrTokenString());
    }
    Advance();
    while (m_token == TOKEN_REQUIRES) {
        ParseDependency(file);
    }
    while (m_token != TOKEN_EOF) {
        ParseDeclaration(file);
    }
    return(file);
}

void Parser::ParseDependency(AstFile *file)
{
    AstDependency   *dep;
    string          path;

    if (Advance() != TOKEN_STRING) {
        Error("expecting the required package path");
    } 
    path = m_lexer->CurrTokenString();
    if (Advance() == TOKEN_COMMA) {
        if (Advance() != TOKEN_NAME) {
            Error("expecting the required package local name");
        }
        dep = new AstDependency(path.c_str(), m_lexer->CurrTokenString());
        Advance();
    } else {
        dep = new AstDependency(path.c_str(), NULL);
    }
    file->AddDependency(dep);
}

void Parser::ParseDeclaration(AstFile *file)
{
    IAstNode    *node = NULL;

    if (m_token == TOKEN_PUBLIC) {
        //m_builder
        Advance();
    }
    switch (m_token) {
    case TOKEN_TYPE:
        Error("type declaration not yet implemented");
        break;
    case TOKEN_VAR:
        node = ParseVar();
        break;
    case TOKEN_CONST:
        Error("const declaration not yet implemented");
        break;
    case TOKEN_FUNC:
        node = ParseFunctionDeclaration();
        break;
    default:
        Error("Expected a declaration: type, var, const, func, struct, class, interface, enum");
    }
    if (node != NULL) file->AddNode(node);
}

IAstNode *Parser::ParseVar(void)
{
    VarDeclaration *var;

    if (Advance() != TOKEN_NAME) {
        Error("expecting the variable name");
    }
    var = new VarDeclaration(m_lexer->CurrTokenString());
    Advance();
    if (m_token == TOKEN_VOLATILE) {
        var->SetVolatile();
        Advance();
    }
    var->SetType(ParseTypeSpecification());
    if (m_token == TOKEN_EQUAL) {
        Advance();
        var->SetIniter(ParseIniter());
    }
    return(var);
}

/*
type_specification ::= base_type | <type_name> | <pkg_name>.<type_name> |
                        map '(' type_specification ')' type_specification |
                        {‘[‘ ([const_expression]) ‘]’} type_specification |
                        matrix {‘[‘ ([ expression[..expression] ]) ‘]’} type_specification |
                        [const] [weak] ‘*’ type_specification |
                        function_type
*/
IAstNode *Parser::ParseTypeSpecification(void)
{
    IAstNode *node;

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
        node = new AstBaseType(m_token);
        Advance();
        break;
    case TOKEN_NAME:
        {
            string part1, part2;
            ParseFullName(&part1, &part2);
            if (part2.length() > 0) {
                node = new AstQualifiedType(part1.c_str(), part2.c_str());
            } else {
                node = new AstNamedType(part1.c_str());
            }
        }
        break;
    case TOKEN_MAP:
        Error("Maps not yet supported");
        break;
    case TOKEN_SQUARE_OPEN:
        node = ParseIndices(/*ismatrix = */false);
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
    return(node);
}

void Parser::ParseFullName(string *part1, string *part2)
{
    if (m_token != TOKEN_NAME) {
        Error("Name expected");
    }
    *part1 = m_lexer->CurrTokenString();
    if (Advance() == TOKEN_DOT) {
        if (Advance() == TOKEN_NAME) {
            *part2 = m_lexer->CurrTokenString();
            Advance();
        } else {
            Error("Expecting the second part of a qualifyed name");
        }
    } else {
        *part2 = "";
    }
}

AstArrayOrMatrixType *Parser::ParseIndices(bool ismatrix)
{
    Error("Arrays not yet supported");
    return(NULL);
}

AstIniter *Parser::ParseIniter(void)
{
    Error("Initer not yet supported");
    return(NULL);
}

//func_definition ::= func func_fullname function_type block
FuncDeclaration *Parser::ParseFunctionDeclaration(void)
{
    string          name1, name2;
    FuncDeclaration *node;

    Advance();
    ParseFullName(&name1, &name2);
    node = new FuncDeclaration(name1.c_str(), name2.c_str());
    node->AddType(ParseFunctionType());
    node->AddBlock(ParseBlock());
    return(node);
}

//function_type ::= [pure] argsdef type_specification
AstFuncType *Parser::ParseFunctionType(void)
{
    AstFuncType *node;

    if (m_token == TOKEN_PURE) {
        node = new AstFuncType(true);
        Advance();
    } else {
        node = new AstFuncType(false);
    }
    ParseArgsDef(node);
    node->SetReturnType(ParseTypeSpecification());
    return(node);
}

// argsdef ::=  ‘(’ ( single_argdef ) [',' ...] ‘)’ | '(' … ')'
void Parser::ParseArgsDef(AstFuncType *desc)
{
    if (m_token != TOKEN_ROUND_OPEN) {
        Error("Expected (");
    }
    if (Advance() != TOKEN_ROUND_CLOSE) {
        do {
            if (m_token == TOKEN_ETC) {
                desc->SetVarArgs();
                Advance();  // to )
                break;      // because we dont' allow additional arguments
            } else {
                desc->AddArgument(ParseSingleArgDef());
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
AstArgumentDecl *Parser::ParseSingleArgDef(void)
{
    Token direction = m_token;
    AstArgumentDecl *node;

    switch (m_token) {
    case TOKEN_IN:
    case TOKEN_OUT:
    case TOKEN_IO:
        Advance();
        break;
    }
    if (m_token != TOKEN_NAME) {
        Error("expecting the argument name");
    }
    node = new AstArgumentDecl(direction, m_lexer->CurrTokenString());
    Advance();
    node->AddType(ParseTypeSpecification());
    if (m_token == TOKEN_EQUAL) {
        Advance();
        node->AddIniter(ParseIniter());
    }
    return(node);
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
AstBlock *Parser::ParseBlock(void)
{
    if (m_token != TOKEN_CURLY_OPEN) {
        Error("Expecting a block (i.e. expecting '{' )");
    }
    AstBlock *node = new AstBlock();
    Advance();
    while (m_token != TOKEN_CURLY_CLOSE) {
        switch (m_token) {
        case TOKEN_VAR:
            node->AddItem(ParseVar());
            break;
        case TOKEN_CONST:
            Error("const not yet supported");
            break;
        case TOKEN_CURLY_OPEN:
            node->AddItem(ParseBlock());
            break;
        default:
            node->AddItem(ParseLeftTermStatement());
            break;
            //Error("In a funcion block you can only place: statements, vars/const declarations, blocks");
        }
    }
    Advance();
    return(node);
}

//(left_term)‘ = ’(expression) |
//left_term update_operator expression |
//left_term++ | left_term-- |
//functioncall |    ====>>> i.e. a left term
IAstNode *Parser::ParseLeftTermStatement(void)
{
    bool                done;
    vector<IAstNode*>   assignee, expressions;
    IAstNode            *node = NULL;
    Token               token;

    do {
        assignee.push_back(ParseLeftTerm());
        if (m_token == TOKEN_COMMA) {
            Advance();
        } else {
            done = true;
        }
    } while (!done);
    if (m_token != TOKEN_ASSIGN && assignee.size() > 1) {
        Error("Only plain assignments can involve multiple assignee");
    }
    token = m_token;
    switch (m_token) {
    case TOKEN_ASSIGN:
        done = false;
        Advance();
        do {
            expressions.push_back(ParseExpression());
            if (assignee.size() == expressions.size() && m_token == TOKEN_COMMA) {
                Error("There are more values than variables in assignment");
            } else if (assignee.size() != expressions.size() && m_token != TOKEN_COMMA) {
                Error("There are more variables than values in assignment");
            }
            if (m_token == TOKEN_COMMA) {
                Advance();
            }
        } while (expressions.size() < assignee.size());
        node = new AstAssignment(&assignee, &expressions);
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
        node = new AstUpdate(token, assignee[0], ParseExpression());
        break;
    case TOKEN_INC:
    case TOKEN_DEC:
        Advance();
        node = new AstIncDec(token, assignee[0]);
        break;
    default:
        node = assignee[0];
        if (node->GetType() != ANT_FUNCALL) {
            Error("expression or part of it has no effects");
        }
        break;      // let's assume it is a function call.
    }
    return(node);
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
IAstNode *Parser::ParseExpression(void)
{
    IAstNode    *node = NULL;
    Token       subtype;

    switch (m_token) {
    case TOKEN_NULL:
    case TOKEN_FALSE:
    case TOKEN_TRUE:
        node = new AstExpressionLeaf(m_token, "");
        Advance();
        break;
    case TOKEN_LITERAL_STRING:
    case TOKEN_LITERAL_UINT:
    case TOKEN_LITERAL_FLOAT:
    case TOKEN_LITERAL_IMG:
        node = new AstExpressionLeaf(m_token, m_lexer->CurrTokenVerbatim());
        Advance();
        break;
    case TOKEN_SIZEOF:
    case TOKEN_DIMOF:
        subtype = m_token;
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting (");
        }
        Advance();
        node = new AstUnop(subtype, ParseLeftTerm());
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
            subtype = m_token;
            if (Advance() != TOKEN_ROUND_CLOSE) {
                Error("Expecting )");
            }
            Advance();
            node = new AstUnop(subtype, ParseExpression());
            break;
        default:
            node = new AstUnop(TOKEN_ROUND_OPEN, ParseExpression());
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
        subtype = m_token;
        Advance();
        node = new AstUnop(subtype, ParseExpression());
        break;
    default:
        node = ParseLeftTerm("Expecting an expression");
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
        subtype = m_token;
        Advance();
        node = new AstBinop(subtype, node, ParseExpression());
    }
    return(node);
}

//left_term ::= <var_name> | prefixexp ‘[’ index_or_rage ‘]’ | prefixexp ‘.’ <name> |
//              functioncall | ‘(’ prefixexp ‘)’ | '*'left_term
IAstNode *Parser::ParseLeftTerm(const char *errmess)
{
    IAstNode    *node = NULL;

    switch (m_token) {
    case TOKEN_MPY:
        Advance();
        node = new AstUnop(TOKEN_MPY, ParseLeftTerm());
        break;
    case TOKEN_ROUND_OPEN:
        Advance();
        node = new AstUnop(TOKEN_ROUND_OPEN, ParseLeftTerm());
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expcting )");
        }
        break;
    case TOKEN_NAME:
        {
            node = new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString());
            bool done = false;
            while (!done) {
                switch (Advance()) {
                case TOKEN_SQUARE_OPEN:
                    ParseIndices();
                    break;
                case TOKEN_DOT:
                    if (Advance() != TOKEN_NAME) {
                        Error("Expecting field name");
                    }
                    node = new AstBinop(TOKEN_DOT, node, new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString()));
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
    return(node);
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