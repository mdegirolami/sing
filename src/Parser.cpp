#include <string.h>
#include "Parser.h"
#include "helpers.h"

namespace SingNames {

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
    AstFile *file = NULL;

    has_errors_ = on_error_ = false;
    errors_.Reset();
    try {
        ParseFile(&file);
    } catch(...) {
        has_errors_ = true;
    }
    if (file != NULL && has_errors_) {
        delete file;
        return(NULL);
    }
    return(file);
}

void Parser::ParseFile(AstFile **file)
{
    bool    done = false;

    if (Advance() != TOKEN_PACKAGE) {
        Error("Expecting a Package declaration");
    }
    if (Advance() != TOKEN_NAME) {
        Error("Expecting the package name");
    } else {
        *file = new AstFile(m_lexer->CurrTokenString());
    }
    try {
        Advance();
    } catch(...) {
        SkipToNextDeclaration();
    }
    while (m_token == TOKEN_REQUIRES) {
        try {
            ParseDependency(*file);
        }
        catch (...) {
            SkipToNextDeclaration();
        }
    }
    while (m_token != TOKEN_EOF) {
        try {
            ParseDeclaration(*file);
        } catch (...) {
            SkipToNextDeclaration();
        }
    }
}

void Parser::ParseDependency(AstFile *file)
{
    if (Advance() != TOKEN_LITERAL_STRING) {
        Error("Expecting the required package path");
    } 
    /*
    AstDependency   *dep;
    string          path;

    path = m_lexer->CurrTokenString();
    if (Advance() == TOKEN_COMMA) {
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the required package local name");
        }
        dep = new AstDependency(path.c_str(), m_lexer->CurrTokenString());
        Advance();
    } else {
        dep = new AstDependency(path.c_str(), NULL);
    }
    */
    file->AddDependency(new AstDependency(m_lexer->CurrTokenString(), NULL));
    Advance();
    //CheckSemicolon();
}

void Parser::ParseDeclaration(AstFile *file)
{
    IAstNode    *node = NULL;

    if (m_token == TOKEN_PUBLIC) {
        Advance();
    }
    switch (m_token) {
    case TOKEN_TYPE:
        node  = ParseType();
        break;
    case TOKEN_VAR:
        node = ParseVar();
        break;
    case TOKEN_CONST:
        node = ParseConst();
        break;
    case TOKEN_FUNC:
        node = ParseFunctionDeclaration();
        break;
    default:
        Error("Expecting a declaration: type, var, const, func, struct, class, interface, enum");
    }
    file->AddNode(node);
}

VarDeclaration *Parser::ParseVar(void)
{
    VarDeclaration *var = NULL;
    bool            type_defined = false;

    if (Advance() != TOKEN_NAME) {
        Error("Expecting the variable name");
    }
    var = new VarDeclaration(m_lexer->CurrTokenString());
    try {
        Advance();
        if (m_token == TOKEN_VOLATILE) {
            var->SetVolatile();
            Advance();
        }
        if (m_token != TOKEN_ASSIGN && m_token != TOKEN_SEMICOLON) {
            var->SetType(ParseTypeSpecification());
            type_defined = true;
        }
        if (m_token == TOKEN_ASSIGN) {
            Advance();
            var->SetIniter(ParseIniter());
            type_defined = true;
        }
        if (!type_defined) {
            Error("You must either declare the variable type or use an initializer");
        }
        CheckSemicolon();
    } catch(...) {
        delete var;
        throw;
    }
    return(var);
}

ConstDeclaration *Parser::ParseConst(void)
{
    ConstDeclaration *node;

    if (Advance() != TOKEN_NAME) {
        Error("Expecting the const name");
    }
    node = new ConstDeclaration(m_lexer->CurrTokenString());
    try {
        Advance();
        if (m_token != TOKEN_ASSIGN) {
            node->SetType(ParseTypeSpecification());
        }
        if (m_token == TOKEN_ASSIGN) {
            Advance();
            node->SetIniter(ParseIniter());
        } else {
            Error("Const requires an initializer !");
        }
        CheckSemicolon();
    } catch(...) {
        delete node;
        throw;
    }
    return(node);
}

TypeDeclaration *Parser::ParseType(void)
{
    TypeDeclaration *node;

    if (Advance() != TOKEN_NAME) {
        Error("Expecting the type name");
    }
    node = new TypeDeclaration(m_lexer->CurrTokenString());
    try {
        Advance();
        node->SetType(ParseTypeSpecification());
        if (m_token == TOKEN_ASSIGN) {
            Error("Can't initialize a type !");
        }
        CheckSemicolon();
    } catch(...) {
        delete node;
        throw;
    }
    return(node);
}

//func_definition ::= func func_fullname function_type block
FuncDeclaration *Parser::ParseFunctionDeclaration(void)
{
    string          name1, name2;
    FuncDeclaration *node;

    if (Advance() != TOKEN_NAME) {
        Error("Expecting the function name");
    }
    name1 = m_lexer->CurrTokenString();
    if (Advance() == TOKEN_DOT) {
        if (Advance() == TOKEN_NAME) {
            name2 = m_lexer->CurrTokenString();
            Advance();
        } else {
            Error("Expecting a member function name after the member selector '.'");
        }
    }
    node = new FuncDeclaration(name1.c_str(), name2.c_str());
    try {
        node->AddType(ParseFunctionType());
        node->AddBlock(ParseBlock());
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
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
    IAstNode *node = NULL;

    try {
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
                string part1 = m_lexer->CurrTokenString();
                if (Advance() == TOKEN_DOT) {
                    node = new AstQualifiedType(part1.c_str());
                    do {
                        if (Advance() != TOKEN_NAME) {
                            Error("Expecting the next portion of a qualifyed name");
                        }
                        ((AstQualifiedType*)node)->AddNameComponent(m_lexer->CurrTokenString());
                    } while (Advance() == TOKEN_DOT);
                } else {
                    node = new AstNamedType(part1.c_str());                    
                }
            }
            break;
        case TOKEN_MAP:
            {
                if (Advance() != TOKEN_ROUND_OPEN) {
                    Error("Expecting '('");
                }
                Advance();
                node = ParseTypeSpecification();
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')'");
                }
                Advance();
                node = new AstMapType(node, ParseTypeSpecification());
            }
            break;
        case TOKEN_SQUARE_OPEN:
            node = ParseIndices(/*ismatrix = */false);
            break;
        case TOKEN_MATRIX:
            if (Advance() != TOKEN_SQUARE_OPEN) {
                Error("Expecting '['");
            }
            node = ParseIndices(/*ismatrix = */true);
            break;
        case TOKEN_CONST:
        case TOKEN_WEAK:
        case TOKEN_MPY:
            {
                bool isconst = false;
                bool isweak = false;

                while (m_token == TOKEN_CONST || m_token == TOKEN_WEAK) {
                    if (m_token == TOKEN_CONST) {
                        isconst = true;
                    } else {
                        isweak = true;
                    }
                    Advance();
                }
                if (m_token != TOKEN_MPY) {
                    Error("Expecting '*'");
                }
                Advance();
                node = new AstPointerType(isconst, isweak, ParseTypeSpecification());
            }
            break;
        case TOKEN_ROUND_OPEN:
        case TOKEN_PURE:
            node = ParseFunctionType();
            break;
        default:
            Error("Invalid type declaration");
            break;
        }
    } catch(...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

AstArrayOrMatrixType *Parser::ParseIndices(bool ismatrix)
{
    AstArrayOrMatrixType    *node = NULL;

    try {
        node = new AstArrayOrMatrixType(ismatrix);
        while (m_token == TOKEN_SQUARE_OPEN) {
            do {
                Advance();
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                    node->SetDimensionExpression(ParseExpression());
                    if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                        Error("Expecting ']' or ','");
                    }
                } else {
                    node->SetDimensionExpression(NULL);
                }
            } while (m_token == TOKEN_COMMA);
            Advance();
        }
        node->SetElementType(ParseTypeSpecification());
    } catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

// initer :: = expression | ‘{ ‘(initer) ‘ }’
IAstNode *Parser::ParseIniter(void)
{
    if (m_token == TOKEN_CURLY_OPEN) {
        AstIniter *node = new AstIniter();
        try {
            do {
                Advance();
                node->AddElement(ParseIniter());
            } while (m_token == TOKEN_COMMA);
            if (m_token != TOKEN_CURLY_CLOSE) {
                Error("Expecting '}'");
            }
            Advance();  // absorb }
        } catch (...) {
            delete node;
            throw;
        }
        return(node);
    }
    return(ParseExpression());
}

//function_type ::= [pure] argsdef type_specification
AstFuncType *Parser::ParseFunctionType(void)
{
    AstFuncType *node = NULL;

    try {
        if (m_token == TOKEN_PURE) {
            node = new AstFuncType(true);
            Advance();
        } else {
            node = new AstFuncType(false);
        }
        ParseArgsDef(node);
        node->SetReturnType(ParseTypeSpecification());
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
}

// argsdef ::=  ‘(’ ( single_argdef ) [',' ...] ‘)’ | '(' … ')'
void Parser::ParseArgsDef(AstFuncType *desc)
{
    if (m_token != TOKEN_ROUND_OPEN) {
        Error("Expecting '('");
    }
    if (Advance() != TOKEN_ROUND_CLOSE) {
        while (true) {
            if (m_token == TOKEN_ETC) {
                desc->SetVarArgs();
                Advance();
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')' no other arguments allowed after ellipsis");
                }
                break;
            } else {
                desc->AddArgument(ParseSingleArgDef());
            }
            if (m_token == TOKEN_ROUND_CLOSE) {
                break;
            } else if (m_token != TOKEN_COMMA) {
                Error("Expecting ','");
            }
            Advance();  // absorb ','
        }
    }
    Advance();  // absorb ')'
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
        Error("Expecting the argument name");
    }
    node = new AstArgumentDecl(direction, m_lexer->CurrTokenString());
    try {
        Advance();
        node->AddType(ParseTypeSpecification());
        if (m_token == TOKEN_ASSIGN) {
            Advance();
            node->AddIniter(ParseIniter());
        }
    } catch (...) {
        delete node;
        throw;
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
        Error("Expecting a block (i.e. Expecting '{' )");
    }
    AstBlock *node = new AstBlock();
    try {
        Advance();
        while (m_token != TOKEN_CURLY_CLOSE) {
            try {
                switch (m_token) {
                case TOKEN_VAR:
                    node->AddItem(ParseVar());
                    break;
                case TOKEN_CONST:
                    node->AddItem(ParseConst());
                    break;
                case TOKEN_CURLY_OPEN:
                    node->AddItem(ParseBlock());
                    break;
                case TOKEN_WHILE:      
                    node->AddItem(ParseWhile());
                    break;
                case TOKEN_IF:
                    node->AddItem(ParseIf());
                    break;
                case TOKEN_FOR:
                    node->AddItem(ParseFor());
                    break;
                case TOKEN_BREAK:
                case TOKEN_CONTINUE:
                    node->AddItem(new AstSimpleStatement(m_token));
                    Advance();
                    CheckSemicolon();
                    break;
                case TOKEN_RETURN:
                    if (Advance() == TOKEN_ROUND_OPEN) {
                        Advance();
                        node->AddItem(new AstReturn(ParseExpression()));
                        if (m_token != TOKEN_ROUND_CLOSE) {
                            Error("Expecting ')'");
                        }
                        Advance();
                    } else {
                        node->AddItem(new AstReturn(NULL));
                    }
                    CheckSemicolon();
                    break;
                case TOKEN_INC:
                case TOKEN_DEC:
                    {
                        Token token = m_token;
                        Advance();
                        node->AddItem(new AstIncDec(token, ParseLeftTerm()));
                        CheckSemicolon();
                    }
                    break;
                default:
                    node->AddItem(ParseLeftTermStatement());
                    break;
                }
            } catch (...) {
                if (!SkipToNextStatement() || m_token == TOKEN_EOF) {
                    throw;
                }
            }
        }
        Advance();
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
}

//(left_term)‘ = ’(expression) |
//left_term update_operator expression |
//left_term++ | left_term-- |
//functioncall |    ====>>> i.e. a left term
IAstNode *Parser::ParseLeftTermStatement(void)
{
    bool                done = false;
    vector<IAstNode*>   assignee, expressions;
    IAstNode            *node = NULL;
    Token               token;

    try {
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
            assignee.clear();       // so in case of exception they are not deleted twice.
            expressions.clear();
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
            Advance();
            node = new AstUpdate(token, assignee[0], ParseExpression());
            assignee.clear();
            break;
        case TOKEN_INC:
        case TOKEN_DEC:
            Advance();
            node = new AstIncDec(token, assignee[0]);
            assignee.clear();
            break;
        default:
            node = assignee[0];
            assignee.clear();
            if (node->GetType() != ANT_FUNCALL) {
                Error("Expression or part of it has no effects");
            }
            break;      // let's assume it is a function call.
        }
        CheckSemicolon();
    } catch(...) {
        if (node != NULL) delete node;
        for (int ii = 0; ii < (int)assignee.size(); ++ii) {
            delete assignee[ii];
        }
        for (int ii = 0; ii < (int)expressions.size(); ++ii) {
            delete expressions[ii];
        }
        throw;
    }
    return(node);
}

/*
expression :: = prefix_expression | expression binop expression

binop ::=  ‘+’ | ‘-’ | ‘*’ | ‘/’ | ‘^’ | ‘%’ | 
	  ‘&’ | ‘|’ | ‘>>’ | ‘<<’ |
	  ‘<’ | ‘<=’ | ‘>’ | ‘>=’ | ‘==’ | ‘!=’ | 
	  xor | && | || 
*/
IAstNode *Parser::ParseExpression(int max_priority)
{
    IAstNode    *nodes[Lexer::max_priority + 2];
    Token       subtype[Lexer::max_priority + 2];
    int         priorities[Lexer::max_priority + 2];
    int         num_nodes = 0;
    int         num_ops = 0;
    int         priority;

    try {
        do {
            nodes[num_nodes++] = ParsePrefixExpression();
            priority = m_lexer->GetBinopPriority(m_token);
            while (num_ops > 0 && priority >= priorities[num_ops - 1]) {
                --num_nodes;
                --num_ops;
                nodes[num_nodes - 1] = new AstBinop(subtype[num_ops], nodes[num_nodes - 1], nodes[num_nodes]);
            }
            if (priority <= Lexer::max_priority) {
                subtype[num_ops] = m_token;
                priorities[num_ops++] = priority;
                Advance();
            }
        } while (priority <= Lexer::max_priority);
    } catch (...) {
        for (int ii = 0; ii < num_nodes; ++ii) {
            delete nodes[ii];
        }
        throw;
    }
    return(nodes[0]);
}

//
// an expression made only by a name with prefix and postfix operators.
// (no binary operators)
//
//prefix_expression :: = null | false | true | err_ok | err_bounds |
//          <Numeral> | <LiteralString> | <LiteralComplex> |
//          left_term | sizeof '(' left_term ')' | sizeof '(' type_specification ')' | dimof '(' left_term ')' |
//          '(' base_type ')' prefix_expression |
//          unop prefix_expression | ‘(’ expression ‘)’
//
// unop :: = ‘ - ’ | ‘!’ | ‘~’ | '&' | '*'
//
IAstNode *Parser::ParsePrefixExpression(void)
{
    IAstNode    *node = NULL;
    Token       subtype;

    try {
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
            if (Advance() != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
            }
            Advance();
            if (m_token == TOKEN_NAME || m_token == TOKEN_MPY) {
                node = new AstUnop(TOKEN_SIZEOF, ParseLeftTerm());
            } else {
                node = new AstUnop(TOKEN_SIZEOF, ParseTypeSpecification());
            }
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            Advance();
            break;
        case TOKEN_DIMOF:
            if (Advance() != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
            }
            Advance();
            node = new AstUnop(TOKEN_DIMOF, ParseLeftTerm());
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
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
                    Error("Expecting ')'");
                }
                Advance();
                node = new AstUnop(subtype, ParsePrefixExpression());
                break;
            default:
                node = new AstUnop(TOKEN_ROUND_OPEN, ParseExpression());
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')'");
                }
                Advance();
                break;
            }
            break;
        case TOKEN_MINUS:
        case TOKEN_PLUS:
        case TOKEN_AND:
        case TOKEN_NOT:
        case TOKEN_LOGICAL_NOT:
        //case TOKEN_DOT:
            subtype = m_token;
            Advance();
            node = new AstUnop(subtype, ParsePrefixExpression());
            break;
        default:
            node = ParseLeftTerm("Expecting an expression");
            break;
        }
    } catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

//left_term :: = <var_name> | left_term ‘[’ indices_or_rages ‘]’ | left_term ‘.’ <name> |
//              functioncall | ‘(’ left_term ‘)’ | '*'left_term
IAstNode *Parser::ParseLeftTerm(const char *errmess)
{
    IAstNode    *node = NULL;

    try {
        switch (m_token) {
        case TOKEN_MPY:
            Advance();
            node = new AstUnop(TOKEN_MPY, ParseLeftTerm());
            break;
        case TOKEN_ROUND_OPEN:
            Advance();
            node = new AstUnop(TOKEN_ROUND_OPEN, ParseLeftTerm());
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            break;
        case TOKEN_NAME:
            {
                node = new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString());
                Advance();
                bool done = false;
                while (!done) {
                    switch (m_token) {
                    case TOKEN_SQUARE_OPEN:
                        node = new AstIndexing(node);
                        ParseRangesOrIndices((AstIndexing*)node);
                        break;
                    case TOKEN_DOT:
                        if (Advance() != TOKEN_NAME) {
                            Error("Expecting the field name");
                        }
                        node = new AstBinop(TOKEN_DOT, node, new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString()));
                        Advance();
                        break;
                    case TOKEN_ROUND_OPEN:
                        node = new AstFunCall(node);
                        ParseArguments((AstFunCall*)node);
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
    } catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

void Parser::ParseRangesOrIndices(AstIndexing *node)
{
    IAstNode *lower;
    
    while (m_token == TOKEN_SQUARE_OPEN) {
        do {
            if (Advance() == TOKEN_COLON) {
                Advance();
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                    node->AddARange(NULL, ParseExpression());
                } else {
                    node->AddARange(NULL, NULL);
                }
            } else {
                lower = ParseExpression();
                if (m_token == TOKEN_COLON) {
                    Advance();
                    if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                        node->AddARange(lower, ParseExpression());
                    } else {
                        node->AddARange(lower, NULL);
                    }
                } else {
                    node->AddAnIndex(lower);
                }
            }
            if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                Error("Expecting ']' or ','");
            }
        } while (m_token != TOKEN_SQUARE_CLOSE);
        Advance();
    }
}

void Parser::ParseArguments(AstFunCall *node)
{
    AstArgument *argument;

    do {
        if (Advance() != TOKEN_COMMA) {
            argument = new AstArgument(ParseExpression());
            node->AddAnArgument(argument);
            if (m_token == TOKEN_COLON) {
                if (Advance() != TOKEN_NAME) {
                    Error("Expecting the parameter name");
                } 
                argument->AddName(m_lexer->CurrTokenString());
                Advance();
            }
        } else {
            node->AddAnArgument(NULL);
        }
        if (m_token != TOKEN_ROUND_CLOSE && m_token != TOKEN_COMMA) {
            Error("Expecting ')' or ','");
        }
    } while (m_token == TOKEN_COMMA);
    Advance();
}

AstWhile *Parser::ParseWhile(void)
{
    IAstNode    *expression;

    if (Advance() != TOKEN_ROUND_OPEN) {
        Error("Expecting '('");
    }
    Advance();
    expression = ParseExpression();
    if (m_token != TOKEN_ROUND_CLOSE) {
        Error("Expecting ')'");
    }
    Advance();
    return(new AstWhile(expression, ParseBlock()));
}

AstIf *Parser::ParseIf(void)
{
    AstIf *node = new AstIf();
    try {
        do {
            if (Advance() != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
            }
            Advance();
            node->AddExpression(ParseExpression());
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            Advance();
            node->AddBlock(ParseBlock());
            if (m_token != TOKEN_ELSE) break;   // done !
            if (Advance() != TOKEN_IF) {
                node->SetDefaultBlock(ParseBlock());
                break;
            }
        } while (true);
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
}

// for_statement := for '(' [<name>','] <name> in for_range ')' block
// for_range := expression [':' expression [step expression]]  
AstFor *Parser::ParseFor(void)
{
    string      first_name;
    AstFor      *node = new AstFor();
    IAstNode    *first_expr;

    try {
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
        }
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the iterator or index name");
        }
        first_name = m_lexer->CurrTokenString();
        if (Advance() == TOKEN_COMMA) {
            if (Advance() != TOKEN_NAME) {
                Error("Expecting the iterator name");
            }
            node->SetIndexName(first_name.c_str());
            node->SetIteratorName(m_lexer->CurrTokenString());
            Advance();
        } else {
            node->SetIteratorName(first_name.c_str());
        }
        if (m_token != TOKEN_IN) {
            Error("Expecting 'in' followed by the for iteration range");
        }
        Advance();
        first_expr = ParseExpression();
        if (m_token == TOKEN_COLON) {
            node->SetLowBound(first_expr);
            Advance();
            node->SetHightBound(ParseExpression());
            if (m_token == TOKEN_STEP) {
                Advance();
                node->SetStep(ParseExpression());
            }
        } else {
            node->SetTheSet(first_expr);
        }
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
        }
        Advance();
        node->SetBlock(ParseBlock());
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
}

void Parser::CheckSemicolon(void)
{
    if (m_token != TOKEN_SEMICOLON) {
        Error("Missing ';'");
    }
    Advance();
}

Token Parser::Advance(void)
{
    do {
        try {
            m_token = m_lexer->Advance();
        } catch (ParsingException ex) {
            SetError(ex.description, ex.row, ex.column);
            m_lexer->ClearError();
            throw;
        } catch(...) {
            Error("Lexer error");
        }
    } while (m_token == TOKEN_COMMENT || m_token == TOKEN_INLINE_COMMENT);
    return(m_token);
}

void Parser::Error(const char *message)
{
    int line, col;

    line = m_lexer->CurrTokenLine();
    col = m_lexer->CurrTokenColumn() + 1;
    SetError(message, line, col);
    throw(ParsingException(1000, line, col, message));
}

void Parser::SetError(const char *message, int row, int column)
{
    char fullmessage[600];

    // errors happening while trying to recover are ignored.
    if (on_error_) {
        return;
    }
    if (strlen(message) > 512) {
        errors_.AddName(message);
    } else {
        sprintf(fullmessage, "line: %d \tcolumn: %d \t%s", row, column, message);
        errors_.AddName(fullmessage);
    }
    has_errors_ = true;
    on_error_ = true;
}

bool Parser::SkipToNextStatement(void)
{
    int     indent = 0;
    bool    success = true;

    while (m_token != TOKEN_EOF) {
        if (m_token == TOKEN_CURLY_OPEN) {
            ++indent;
        } else if (m_token == TOKEN_CURLY_CLOSE) {
            if (--indent <= 0) {
                success = indent == 0;
                break;
            }
        } else if (indent == 0 && m_token == TOKEN_SEMICOLON) {
            break;
        } else if (OutOfFunctionToken()) {
            on_error_ = false;
            return(false);      // exit here: don't absorb the token
        }
        SkipToken();
    }
    if (Advance() == TOKEN_SEMICOLON) {
        Advance();
    }
    on_error_ = false;
    return(success);
}

void Parser::SkipToNextDeclaration(void)
{
    int indent = 0;

    while (m_token != TOKEN_EOF) {
        if (m_token == TOKEN_CURLY_OPEN) {
            ++indent;
        } else if (m_token == TOKEN_CURLY_CLOSE) {
            if (--indent < 0) {
                indent = 0;
            }
        } else if (indent == 0 && OnDeclarationToken()) {
            break;
        }
        SkipToken();
    }
    on_error_ = false;
}

bool Parser::OnDeclarationToken(void) 
{
    switch (m_token) {
    case TOKEN_PUBLIC:
    case TOKEN_VAR:
    case TOKEN_CONST:
    case TOKEN_TYPE:
    case TOKEN_FUNC:
    case TOKEN_ENUM:
    case TOKEN_STRUCT:
    case TOKEN_CLASS:
    case TOKEN_INTERFACE:
    case TOKEN_TEMPLATE:
    case TOKEN_EOF:
    case TOKEN_REQUIRES:
        return(true);
    default:
        break;
    }
    return(false);
}

bool Parser::OutOfFunctionToken(void)
{
    switch (m_token) {
    case TOKEN_PUBLIC:
    //case TOKEN_VAR:
    //case TOKEN_CONST:
    case TOKEN_TYPE:
    case TOKEN_FUNC:
    case TOKEN_ENUM:
    case TOKEN_STRUCT:
    case TOKEN_CLASS:
    case TOKEN_INTERFACE:
    case TOKEN_TEMPLATE:
    case TOKEN_EOF:
    case TOKEN_REQUIRES:
        return(true);
    default:
        break;
    }
    return(false);
}

Token Parser::SkipToken(void)
{
    try {
        m_token = m_lexer->Advance();
    } catch (...) {
        // ignore errors while resyncing
    }
    return(m_token);
}

}