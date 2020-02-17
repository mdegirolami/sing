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

AstFile *Parser::ParseAll(ErrorList *errors, bool for_reference)
{
    has_errors_ = on_error_ = false;
    errors_ = errors;
    for_reference_ = for_reference;
    curly_indent_ = 0;
    root_ = new AstFile();
    try {
        ParseFile(root_, for_reference);
    } catch(...) {
        has_errors_ = true;
    }
    if (root_ != NULL && has_errors_) {
        delete root_;
        return(NULL);
    }
    if (!has_errors_) {
        AttachCommentsToNodes();
    }
    return(root_);
}

void Parser::ParseFile(AstFile *file, bool for_reference)
{
    bool    done = false;

    if (Advance() == TOKEN_NAMESPACE) {
        try {
            ParseNamespace(file);
        } catch (...) {
            SkipToNextDeclaration();
        }
    }
    while (m_token == TOKEN_REQUIRES) {
        try {
            ParseDependency(file);
        }
        catch (...) {
            SkipToNextDeclaration();
        }
    }
    while (m_token != TOKEN_EOF) {
        try {
            ParseDeclaration(file, for_reference);
        } catch (...) {
            SkipToNextDeclaration();
        }
    }
}

void Parser::ParseNamespace(AstFile *file)
{
    if (Advance() != TOKEN_NAME) {
        Error("Expecting the namespace name");
    } else {
        string n_space = m_lexer->CurrTokenString();
        while (Advance() == TOKEN_DOT) {
            if (Advance() != TOKEN_NAME) {
                Error("Expecting the next portion of a qualifyed name");
            }
            n_space += '.';
            n_space += m_lexer->CurrTokenString();
        }
        file->SetNamespace(n_space.c_str());
    }
    CheckSemicolon();
}

void Parser::ParseDependency(AstFile *file)
{
    if (Advance() != TOKEN_LITERAL_STRING) {
        Error("Expecting the required package path (remember to enclose it in double quotes !)");
    } 
    AstDependency *dep = new AstDependency(m_lexer->CurrTokenString(), NULL);
    file->AddDependency(dep);
    RecordPosition(dep);

    if (Advance() == TOKEN_COMMA) {
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the required package local name");
        }
        dep->SetLocalPackageName(m_lexer->CurrTokenString());
        Advance();
    }
    UpdateEndPosition(dep);
    CheckSemicolon();
}

void Parser::ParseDeclaration(AstFile *file, bool for_reference)
{
    IAstDeclarationNode *node = NULL;
    bool    is_public = false;

    if (m_token == TOKEN_PUBLIC) {
        Advance();
        is_public = true;
    } else if (for_reference) {
        if (Advance() == TOKEN_NAME) {
            file->AddPrivateSymbol(m_lexer->CurrTokenString());
        } else {
            Error("Expecting the declaration name");
        }
        SkipToNextDeclaration();
        return;
    }
    switch (m_token) {
    case TOKEN_TYPE:
        node  = ParseType();
        break;
    case TOKEN_VAR:
        node = ParseVar();
        break;
    case TOKEN_LET:
        node = ParseConst();
        break;
    case TOKEN_FUNC:
        node = ParseFunctionDeclaration(for_reference);
        break;
    case TOKEN_ENUM:
        node = ParseEnum();
        break;
    case TOKEN_INTERFACE:
        node = ParseInterface();
        break;
    case TOKEN_CLASS:
        node = ParseClass();
        break;
    default:
        Error("Expecting a declaration: type, var, let, fn, class, interface, enum");
    }
    node->SetPublic(is_public);
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
    RecordPosition(var);
    try {
        Advance();
        //if (m_token == TOKEN_VOLATILE) {
        //    var->SetFlags(VF_ISVOLATILE);
        //    Advance();
        //}
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
        UpdateEndPosition(var);
        CheckSemicolon();
    } catch(...) {
        delete var;
        throw;
    }
    return(var);
}

VarDeclaration *Parser::ParseConst(void)
{
    VarDeclaration *node;
    bool            type_defined = false;

    if (Advance() != TOKEN_NAME) {
        Error("Expecting the const name");
    }
    node = new VarDeclaration(m_lexer->CurrTokenString());
    node->SetFlags(VF_READONLY);
    RecordPosition(node);
    try {
        Advance();
        if (m_token != TOKEN_ASSIGN) {
            node->SetType(ParseTypeSpecification());
            type_defined = true;
        }
        if (m_token == TOKEN_ASSIGN) {
            Advance();
            node->SetIniter(ParseIniter());
            type_defined = true;
        }
        if (!type_defined) {
            Error("You must either declare the const type or use an initializer");
        }
        UpdateEndPosition(node);
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
    RecordPosition(node);
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
FuncDeclaration *Parser::ParseFunctionDeclaration(bool skip_body)
{
    string          name1, name2;
    FuncDeclaration *node = NULL;

    try {
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the function name");
        }
        name1 = m_lexer->CurrTokenString();
        node = new FuncDeclaration();
        RecordPosition(node);
        if (Advance() == TOKEN_DOT) {
            if (Advance() == TOKEN_NAME) {
                name2 = m_lexer->CurrTokenString();
                Advance();
            } else {
                Error("Expecting a member function name after the member selector '.'");
            }
        }
        node->SetNames(name1.c_str(), name2.c_str());
        node->AddType(ParseFunctionType(true));
        UpdateEndPosition(node);
        if (skip_body) {
            SkipToNextDeclaration();
        } else {
            node->AddBlock(ParseBlock());
        }
    } catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

//
// enum_decl :: = enum <type_name> ‘ { ‘(enum_element) ‘ }’
// enum_element :: = <label_name>['=' const_expression]
//
TypeDeclaration *Parser::ParseEnum(void)
{
    TypeDeclaration *node = nullptr;
    try {
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the type name");
        }
        node = new TypeDeclaration(m_lexer->CurrTokenString());
        AstEnumType *typenode = new AstEnumType();
        node->SetType(typenode);
        RecordPosition(node);
        RecordPosition(typenode);
        if (Advance() != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
        }
        do {
            if (Advance() != TOKEN_NAME) {
                Error("Expecting an enum case name");
            }
            string the_case = m_lexer->CurrTokenString();
            if (Advance() == TOKEN_ASSIGN) {
                Advance();  // absorb '='
                typenode->AddItem(the_case.c_str(), ParseExpression());
            } else {
                typenode->AddItem(the_case.c_str(), nullptr);
            }
        } while (m_token == TOKEN_COMMA);
        if (m_token != TOKEN_CURLY_CLOSE) {
            Error("Expecting '}'");
        }
        UpdateEndPosition(node);
        Advance();  // absorb '}'
    }
    catch (...) {
        delete node;
        throw;
    }
    return(node);
}

//
//interface_decl :: = interface<if_name> ‘{ ‘(member_function_declaration | interface <interface_name> ';') ‘ }’
//
TypeDeclaration *Parser::ParseInterface(void)
{
    TypeDeclaration *node = nullptr;
    try {
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the type name");
        }
        node = new TypeDeclaration(m_lexer->CurrTokenString());
        AstInterfaceType *typenode = new AstInterfaceType();
        node->SetType(typenode);
        RecordPosition(node);
        RecordPosition(typenode);
        Advance();
        //if (m_token == TOKEN_COLON) {
        //    do {
        //        Advance();  // absorb ':' or ','
        //        node->AddAncestor(ParseNamedType());
        //    } while (m_token == TOKEN_COMMA);
        //}
        if (m_token != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
        }
        Advance();  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE) {
            int recovery_level = curly_indent_;
            FuncDeclaration *fun = nullptr;
            try {
                if (m_token == TOKEN_FUNC) {
                    bool is_mutable = false;
                    Advance();
                    if (m_token == TOKEN_MUT) {
                        is_mutable = true;
                        Advance();
                    } 
                    if (m_token != TOKEN_NAME) {
                        Error("Expecting the function name");
                    }
                    fun = new FuncDeclaration();
                    RecordPosition(fun);
                    fun->SetNames(m_lexer->CurrTokenString(), "");
                    fun->SetMuting(is_mutable);
                    fun->SetPublic(true);
                    Advance();
                    fun->AddType(ParseFunctionType(false));
                    UpdateEndPosition(fun);
                    typenode->AddMember(fun);
                    fun = nullptr;
                    CheckSemicolon();
                } else if (m_token == TOKEN_INTERFACE) {
                    Advance();
                    typenode->AddAncestor(ParseNamedType());
                    CheckSemicolon();
                } else {
                    Error("Expecting 'fn' or 'interface'. Note: public/private qualifiers are not allowed here.");
                }
            }
            catch (...) {
                if (fun != nullptr) delete fun;
                if (!SkipToNextStatement(recovery_level)) {
                    throw;
                }
            }
        };
        UpdateEndPosition(node);
        Advance();  // absorb '}'
    }
    catch (...) {
        delete node;
        throw;
    }
    return(node);
}

//
// class_decl :: = class <type_name> ‘{ ‘(class_section) ‘ }’
// class_section :: = public_section | private_section
// public_section :: = public ':' { var_decl | member_function_declaration }
// private_section :: = private ':' { var_decl | member_function_declaration | if_declaration | fn_delegation }
// member_function_declaration :: = fn <func_name> function_type ';'
// fn_delegation ::= fn <func_name> [ by <var_name> ] ';'
// if_declaration ::= interface <interface_name> by <var_name> ';'
//
TypeDeclaration *Parser::ParseClass(void)
{
    TypeDeclaration *node = nullptr;
    bool public_section = false;
    try {
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the type name");
        }
        node = new TypeDeclaration(m_lexer->CurrTokenString());
        AstClassType *typenode = new AstClassType();
        node->SetType(typenode);
        RecordPosition(node);
        RecordPosition(typenode);
        if (Advance() != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
        }
        Advance();  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE) {
            int recovery_level = curly_indent_;
            FuncDeclaration *fun = nullptr;
            AstNamedType *named = nullptr;
            try {
                if (m_token == TOKEN_PRIVATE) {
                    public_section = false;
                    if (Advance() != TOKEN_COLON) {
                        Error("Expecting ':'");
                    }
                    Advance();
                } else if (m_token == TOKEN_PUBLIC) {
                    public_section = true;
                    if (Advance() != TOKEN_COLON) {
                        Error("Expecting ':'");
                    }
                    Advance();
                } else if (m_token == TOKEN_VAR) {
                    VarDeclaration *var = ParseVar();
                    var->SetPublic(public_section);
                    typenode->AddMemberVar(var);
                } else if (m_token == TOKEN_FUNC) {
                    bool is_mutable = false;
                    Advance();
                    if (m_token == TOKEN_MUT) {
                        is_mutable = true;
                        Advance();
                    } 
                    if (m_token != TOKEN_NAME) {
                        Error("Expecting the function name");
                    }
                    fun = new FuncDeclaration();
                    RecordPosition(fun);
                    fun->SetNames(m_lexer->CurrTokenString(), "");
                    fun->SetMuting(is_mutable);
                    fun->SetPublic(public_section);
                    Advance();
                    if (m_token == TOKEN_BY) {
                        if (!public_section) {
                            Error("You can't delegate a private function");
                        }
                        if (Advance() != TOKEN_NAME) {
                            Error("Expecting the name of the var member implementing the function");
                        }
                        UpdateEndPosition(fun);
                        typenode->AddMemberFun(fun, m_lexer->CurrTokenString());
                        fun = nullptr;
                        Advance();
                    } else {
                        fun->AddType(ParseFunctionType(false));
                        UpdateEndPosition(fun);
                        typenode->AddMemberFun(fun, "");
                        fun = nullptr;
                    }
                    CheckSemicolon();
                } else if (m_token == TOKEN_INTERFACE) {
                    if (!public_section) {
                        Error("All the interfaces must be public");
                    }
                    Advance();
                    named = ParseNamedType();
                    if (m_token == TOKEN_BY) {
                        if (Advance() != TOKEN_NAME) {
                            Error("Expecting the name of the var member implementing the interface");
                        }
                        typenode->AddMemberInterface(named, m_lexer->CurrTokenString());
                        named = nullptr;
                        Advance();
                    } else {
                        typenode->AddMemberInterface(named, "");
                        named = nullptr;
                    }
                    CheckSemicolon();
                } else {
                    Error("Expecting a var/fn/interface declaration or a public/private qualifier");
                }
            }
            catch (...) {
                if (fun != nullptr) delete fun;
                if (named != nullptr) delete named;
                if (!SkipToNextStatement(recovery_level)) {
                    throw;
                }
            }
        }
        UpdateEndPosition(node);
        Advance();  // absorb '}'
    }
    catch (...) {
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
IAstTypeNode *Parser::ParseTypeSpecification(void)
{
    IAstTypeNode *node = NULL;

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
        case TOKEN_BOOL:
        case TOKEN_ERRORCODE:
        case TOKEN_VOID:
            node = new AstBaseType(m_token);
            RecordPosition(node);
            Advance();
            break;
        case TOKEN_NAME:
            node = ParseNamedType();
            break;
        case TOKEN_MAP:
            {
                AstMapType *map = new AstMapType();
                node = map;
                RecordPosition(node);
                if (Advance() != TOKEN_ROUND_OPEN) {
                    Error("Expecting '('");
                }
                Advance();
                map->SetKeyType(ParseTypeSpecification());
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')'");
                }
                Advance();
                map->SetReturnType(ParseTypeSpecification());
            }
            break;
        case TOKEN_SQUARE_OPEN:
            node = ParseIndices();
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
                node = new AstPointerType();
                RecordPosition(node);
                Advance();
                ((AstPointerType*)node)->Set(isconst, isweak, ParseTypeSpecification());
            }
            break;
        case TOKEN_ROUND_OPEN:
        case TOKEN_PURE:
            node = ParseFunctionType(false);
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

AstNamedType *Parser::ParseNamedType(void)
{
    AstNamedType *node = new AstNamedType(m_lexer->CurrTokenString());
    try {
        if (m_token != TOKEN_NAME) {
            Error("Expecting a name");
        }
        AstNamedType *last = node;
        RecordPosition(node);
        while (Advance() == TOKEN_DOT) {
            if (Advance() != TOKEN_NAME) {
                Error("Expecting the next portion of a qualifyed name");
            }
            AstNamedType *curr = new AstNamedType(m_lexer->CurrTokenString());
            RecordPosition(curr);
            last->ChainComponent(curr);
            last = curr;
        }
    }
    catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

AstArrayType *Parser::ParseIndices(void)
{
    AstArrayType    *root = NULL;
    AstArrayType    *last, *curr;

    try {
        while (m_token == TOKEN_SQUARE_OPEN) {
            do {
                if (root == NULL) {
                    root = last = curr = new AstArrayType();
                } else {

                    // chain it now: if we need to delete root we don't cause a leak.
                    curr = new AstArrayType();
                    last->SetElementType(curr);
                    last = curr;
                }
                Advance();
                RecordPosition(curr);
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA && m_token != TOKEN_MPY) {
                    curr->SetDimensionExpression(ParseExpression());
                } else if (m_token == TOKEN_MPY) {
                    curr->SetDynamic(true);
                    Advance();
                    curr->SetRegular(m_token == TOKEN_COMMA);
                }
                UpdateEndPosition(curr);
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                    Error("Expecting ']' or ','");
                }
            } while (m_token == TOKEN_COMMA);
            Advance();  // absorb ']'
        }
        last->SetElementType(ParseTypeSpecification());
    } catch (...) {
        if (root != NULL) delete root;
        throw;
    }
    return(root);
}

// initer :: = expression | ‘{ ‘(initer) ‘ }’
IAstNode *Parser::ParseIniter(void)
{
    if (m_token == TOKEN_CURLY_OPEN) {
        AstIniter *node = new AstIniter();
        RecordPosition(node);
        try {
            do {
                Advance();
                node->AddElement(ParseIniter());
            } while (m_token == TOKEN_COMMA);
            if (m_token != TOKEN_CURLY_CLOSE) {
                Error("Expecting '}'");
            }
            UpdateEndPosition(node);
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
AstFuncType *Parser::ParseFunctionType(bool is_body)
{
    AstFuncType *node = NULL;

    try {
        if (m_token == TOKEN_PURE) {
            node = new AstFuncType(true);
            Advance();
        } else {
            node = new AstFuncType(false);
        }
        RecordPosition(node);
        ParseArgsDef(node, is_body);
        node->SetReturnType(ParseTypeSpecification());
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
}

// argsdef ::=  ‘(’ ( single_argdef ) [',' ...] ‘)’ | '(' … ')'
void Parser::ParseArgsDef(AstFuncType *desc, bool is_function_body)
{
    if (m_token != TOKEN_ROUND_OPEN) {
        Error("Expecting '('");
    }
    if (Advance() != TOKEN_ROUND_CLOSE) {

        // used only inside AddArgument to check if all the args after the first inited one have initers.
        bool mandatory_initer = false;

        while (true) {
            if (m_token == TOKEN_ETC) {
                desc->SetVarArgs();
                Advance();
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')' no other arguments allowed after ellipsis");
                }
                break;
            } else {
                desc->AddArgument(ParseSingleArgDef(is_function_body, &mandatory_initer));
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
VarDeclaration *Parser::ParseSingleArgDef(bool is_function_body, bool *mandatory_initer)
{
    Token direction = m_token;
    VarDeclaration *node;

    switch (m_token) {
    case TOKEN_IN:
    case TOKEN_OUT:
    case TOKEN_IO:
        Advance();
        break;
    default:
        direction = TOKEN_IN;
        break;
        //if (!is_function_body) {
        //    Error("Expecting the argument direction (in, out, io)");
        //}
    }
    if (m_token != TOKEN_NAME) {
        Error("Expecting the argument name");
    }
    node = new VarDeclaration(m_lexer->CurrTokenString());
    node->SetFlags(VF_ISARG);
    if (direction == TOKEN_IN) {
        node->SetFlags(VF_READONLY);
    }
    if (direction == TOKEN_OUT) {
        node->SetFlags(VF_WRITEONLY);
    }
    RecordPosition(node);
    try {
        Advance();
        node->SetType(ParseTypeSpecification());
        if (m_token == TOKEN_ASSIGN) {
            Advance();
            node->SetIniter(ParseIniter());
            *mandatory_initer = true;
        } else if (*mandatory_initer) {
            Error("All arguments following a default arg must have a default value.");
        }
        UpdateEndPosition(node);
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
    RecordPosition(node);
    try {
        Advance();
        while (m_token != TOKEN_CURLY_CLOSE) {
            IAstNode *statement = ParseStatement(true);
            if (statement != nullptr) {
                node->AddItem(statement);
            }
        }
        UpdateEndPosition(node);
        Advance();
    } catch (...) {
        delete node;
        throw;
    }
    return(node);
}

IAstNode *Parser::ParseStatement(bool allow_let_and_var)
{
    IAstNode *node = nullptr;
    int recovery_level = curly_indent_;
    try {
        switch (m_token) {
        case TOKEN_VAR:
            if (allow_let_and_var) {
                node = ParseVar();
            } else {
                Error("No declarations allowed here, only statements please !!");
            }
            break;
        case TOKEN_LET:
            if (allow_let_and_var) {
                node = ParseConst();
            } else {
                Error("No declarations allowed here, only statements please !!");
            }
            break;
        case TOKEN_TYPE:
        case TOKEN_FUNC:
        case TOKEN_ENUM:
        case TOKEN_CLASS:
        case TOKEN_INTERFACE:
            Error("The only declarations allowed in a function are Let and Var");
            break;
        case TOKEN_CURLY_OPEN:
            node = ParseBlock();
            break;
        case TOKEN_WHILE:
            node = ParseWhile();
            break;
        case TOKEN_IF:
            node = ParseIf();
            break;
        case TOKEN_FOR:
            node = ParseFor();
            break;
        case TOKEN_SWITCH:
            node = ParseSwitch();
            break;
        case TOKEN_TYPESWITCH:
            node = ParseTypeSwitch();
            break;
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        {
            AstSimpleStatement *ss = new AstSimpleStatement(m_token);
            node = ss;
            RecordPosition(ss);
            Advance();
            CheckSemicolon();
        }
        break;
        case TOKEN_RETURN:
        {
            AstReturn *ast_ret = new AstReturn();
            node = ast_ret;
            RecordPosition(ast_ret);
            if (Advance() == TOKEN_ROUND_OPEN) {
                Advance();
                ast_ret->AddRetExp(ParseExpression());
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')'");
                }
                Advance();
            }
            UpdateEndPosition(ast_ret);
            CheckSemicolon();
        }
        break;
        case TOKEN_INC:
        case TOKEN_DEC:
        {
            AstIncDec *aid = new AstIncDec(m_token);
            node = aid;
            RecordPosition(aid);
            Advance();
            aid->SetLeftTerm(ParseLeftTerm());
            CheckSemicolon();
        }
        break;
        default:
            node = ParseLeftTermStatement();
            break;
        }
    } catch (...) {
        if (node != nullptr) {
            delete node;
            node = nullptr;
        }
        if (!SkipToNextStatement(recovery_level)) {
            throw;
        }
    }
    return(node);
}

//left_term‘ = ’expression |
//left_term update_operator expression |
//left_term++ | left_term-- |
//functioncall |    ====>>> i.e. a left term
IAstNode *Parser::ParseLeftTermStatement(void)
{
    bool                    done = false;
    IAstExpNode             *assignee = NULL;
    IAstNode                *node = NULL;
    Token                   token;
    PositionInfo            pinfo;

    try {
        assignee = ParseLeftTerm("Expecting a statement");
        FillPositionInfo(&pinfo);   // start with token
        token = m_token;
        switch (m_token) {
        case TOKEN_ASSIGN:
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
            node = new AstUpdate(token, assignee, ParseExpression());
            *(node->GetPositionRecord()) = pinfo;
            assignee = NULL;
            break;
        case TOKEN_INC:
        case TOKEN_DEC:
            node = new AstIncDec(token);
            ((AstIncDec*)node)->SetLeftTerm(assignee);
            *(node->GetPositionRecord()) = pinfo;
            assignee = NULL;
            Advance();
            break;
        default:
            if (assignee->GetType() != ANT_FUNCALL) {
                Error("Expression or part of it has no effects");
            }
            node = assignee;
            assignee = NULL;
            break;      // let's assume it is a function call.
        }
        UpdateEndPosition(node);
        CheckSemicolon();
    } catch(...) {
        if (node != NULL) delete node;
        if (assignee != NULL) delete assignee;
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
IAstExpNode *Parser::ParseExpression()
{
    IAstExpNode     *nodes[Lexer::max_priority + 2];
    Token           subtype[Lexer::max_priority + 2];
    int             priorities[Lexer::max_priority + 2];
    PositionInfo    positions[Lexer::max_priority + 2];
    int             num_nodes = 0;
    int             num_ops = 0;
    int             priority;

    try {
        do {
            nodes[num_nodes++] = ParsePrefixExpression();
            priority = m_lexer->GetBinopPriority(m_token);
            while (num_ops > 0 && priority >= priorities[num_ops - 1]) {
                --num_nodes;
                --num_ops;
                nodes[num_nodes - 1] = new AstBinop(subtype[num_ops], nodes[num_nodes - 1], nodes[num_nodes]);
                *(nodes[num_nodes - 1]->GetPositionRecord()) = positions[num_ops];
            }
            if (priority <= Lexer::max_priority) {
                subtype[num_ops] = m_token;
                FillPositionInfo(positions + num_ops);
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
//           base_type '(' expression ')'|
//          unop prefix_expression | ‘(’ expression ‘)’
//
// unop :: = ‘ - ’ | ‘!’ | ‘~’ | '&'
//
IAstExpNode *Parser::ParsePrefixExpression(void)
{
    IAstExpNode    *node = NULL;

    try {
        switch (m_token) {
        case TOKEN_NULL:
        case TOKEN_FALSE:
        case TOKEN_TRUE:
            node = new AstExpressionLeaf(m_token, "");
            RecordPosition(node);
            Advance();
            break;
        case TOKEN_LITERAL_STRING:
            node = new AstExpressionLeaf(m_token, m_lexer->CurrTokenVerbatim());
            RecordPosition(node);
            while (Advance() == TOKEN_LITERAL_STRING)
            {
                ((AstExpressionLeaf*)node)->AppendToValue("\xff");
                ((AstExpressionLeaf*)node)->AppendToValue(m_lexer->CurrTokenVerbatim());
            };
            break;
        case TOKEN_LITERAL_UINT:
        case TOKEN_LITERAL_FLOAT:
        case TOKEN_LITERAL_IMG:
            node = new AstExpressionLeaf(m_token, m_lexer->CurrTokenVerbatim());
            RecordPosition(node);
            Advance();
            break;
        case TOKEN_SIZEOF:
            node = new AstUnop(TOKEN_SIZEOF);
            RecordPosition(node);
            if (Advance() != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
            }
            Advance();
            if (m_token == TOKEN_NAME || m_token == TOKEN_MPY) {
                ((AstUnop*)node)->SetOperand(ParseLeftTerm());
            } else {
                ((AstUnop*)node)->SetTypeOperand(ParseTypeSpecification());
            }
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            Advance();
            break;
        case TOKEN_DIMOF:
            node = new AstUnop(TOKEN_DIMOF);
            RecordPosition(node);
            if (Advance() != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
            }
            Advance();
            ((AstUnop*)node)->SetOperand(ParseLeftTerm());
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            Advance();
            break;
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
        case TOKEN_BOOL:
            node = new AstUnop(m_token);
            RecordPosition(node);
            if (Advance() != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
            }
            Advance();
            ((AstUnop*)node)->SetOperand(ParseExpression());
            node = CheckForCastedLiterals((AstUnop*)node);
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            Advance();
            break;
        case TOKEN_ROUND_OPEN:
            Advance();
            node = ParseExpression();
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            //} else if (node->GetType() == ANT_EXP_LEAF) {
            //    AstExpressionLeaf *leaf = (AstExpressionLeaf*)node;
            //    if (leaf->subtype_ == TOKEN_NAME) {
            //        Error("Single symbol in bracket have no effect, did you mispell a type name in a cast ?");
            //    }
            //}
            Advance();
            break;
        case TOKEN_MINUS:
        case TOKEN_PLUS:
        case TOKEN_AND:
        case TOKEN_NOT:
        case TOKEN_LOGICAL_NOT:
        //case TOKEN_DOT:
            node = new AstUnop(m_token);
            RecordPosition(node);
            Advance();
            ((AstUnop*)node)->SetOperand(ParsePrefixExpression());
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

//
// Fake Conversions (are actually literal type declarations !!)
//
// a casted literal is not an int/float literal converted but a literal of higher precision
// this rotine converts the tree of i64([-]number), ui64([-]number), f64([-]number), c128([-]real +/-img), c128([-]number) ...
// into a single node of type AstExpressionLeaf, subtype same as the cast type.
//
IAstExpNode *Parser::CheckForCastedLiterals(AstUnop *node)
{
    AstExpressionLeaf   *b0, *b1, *ret = NULL;
    bool                b0_negative, b1_negative;

    switch (node->subtype_) {
    case TOKEN_INT32:
    case TOKEN_UINT32:
    case TOKEN_INT64:
    case TOKEN_UINT64:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        break;
    default:
        return(node);
    }

    bool iscomplex = node->subtype_ == TOKEN_COMPLEX128 || node->subtype_ == TOKEN_COMPLEX64;

    if (node->operand_->GetType() == ANT_BINOP) {
        AstBinop *sum = (AstBinop*)node->operand_;
        if (sum->subtype_ != TOKEN_PLUS && sum->subtype_ != TOKEN_MINUS) {
            return(node);
        }
        if (!iscomplex) {
            return(node);
        }
        b0 = GetLiteralRoot(sum->operand_left_, &b0_negative);
        b1 = GetLiteralRoot(sum->operand_right_, &b1_negative);
        if (b0 != NULL && b1 != NULL) {
            if (sum->subtype_ == TOKEN_MINUS) {
                b1_negative = !b1_negative;
            }
            if (b0->subtype_ != TOKEN_LITERAL_IMG && b1->subtype_ == TOKEN_LITERAL_IMG) {
                ret = new AstExpressionLeaf(node->subtype_, b0->value_.c_str());
                ret->SetRealPartNfo(b0->subtype_ == TOKEN_LITERAL_UINT, b0_negative);
                ret->SetImgValue(b1->value_.c_str(), b1_negative);
            } else if (b1->subtype_ != TOKEN_LITERAL_IMG && b0->subtype_ == TOKEN_LITERAL_IMG) {
                ret = new AstExpressionLeaf(node->subtype_, b1->value_.c_str());
                ret->SetRealPartNfo(b1->subtype_ == TOKEN_LITERAL_UINT, b1_negative);
                ret->SetImgValue(b0->value_.c_str(), b0_negative);
            }
        }
    } else {
        b0 = GetLiteralRoot(node->operand_, &b0_negative);
        if (b0 != NULL) {
            if (b0->subtype_ == TOKEN_LITERAL_IMG && iscomplex) {
                ret = new AstExpressionLeaf(node->subtype_, "0.0");
                ret->SetRealPartNfo(false, false);
                ret->SetImgValue(b0->value_.c_str(), b0_negative);
            } else if (b0->subtype_ != TOKEN_LITERAL_IMG) {
                ret = new AstExpressionLeaf(node->subtype_, b0->value_.c_str());
                ret->SetRealPartNfo(b0->subtype_ == TOKEN_LITERAL_UINT, b0_negative);
            }
        }
    }
    if (ret != NULL) {
        delete node;
        RecordPosition(ret);
        return(ret);
    }
    return(node);
}

AstExpressionLeaf *Parser::GetLiteralRoot(IAstExpNode *node, bool *negative)
{
    *negative = false;
    while (true) {
        AstNodeType type = node->GetType();
        if (type == ANT_UNOP) {
            Token op = ((AstUnop*)node)->subtype_;
            if (op == TOKEN_MINUS) {
                *negative = !*negative;
            } else if (op != TOKEN_PLUS) {
                return(NULL);
            }
            node = ((AstUnop*)node)->operand_;
        } else if (type == ANT_EXP_LEAF) {
            Token valtype = ((AstExpressionLeaf*)node)->subtype_;
            if (valtype == TOKEN_LITERAL_FLOAT || valtype == TOKEN_LITERAL_UINT || valtype == TOKEN_LITERAL_IMG) {
                return((AstExpressionLeaf*)node);
            }
            return(NULL);
        } else {
            return(NULL);
        }       
    }
}

//left_term :: = <var_name> | ‘(’ left_term ‘)’ | '*'left_term
//               left_term ‘[’ indices_or_rages ‘]’ | left_term ‘.’ <name> | left_term '(' arguments ')' | this
IAstExpNode *Parser::ParseLeftTerm(const char *errmess)
{
    IAstExpNode    *node = NULL;

    try {
        switch (m_token) {
        case TOKEN_MPY:
            node = new AstUnop(TOKEN_MPY);
            RecordPosition(node);
            Advance();
            ((AstUnop*)node)->SetOperand(ParseLeftTerm());
            break;
        case TOKEN_ROUND_OPEN:
            Advance();
            node = ParseLeftTerm();
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
            }
            Advance();  // absorb ')'
            break;
        case TOKEN_NAME:
            node = new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString());
            RecordPosition(node);
            Advance();
            break;
        case TOKEN_THIS:
            node = new AstExpressionLeaf(TOKEN_THIS, "");
            RecordPosition(node);
            Advance();
            break;
        default:
            Error(errmess != NULL ? errmess : "Expecting a left term (an assignable expression)");
            break;
        }

        // postfixed ?
        bool done = false;
        while (!done) {
            switch (m_token) {
            case TOKEN_SQUARE_OPEN:
                node = ParseRangesOrIndices(node);
                break;
            case TOKEN_DOT:
                {
                    PositionInfo pnfo;
                    FillPositionInfo(&pnfo);
                    if (Advance() != TOKEN_NAME) {
                        Error("Expecting a field name or a symbol");
                    }
                    AstExpressionLeaf *leaf = new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString());
                    RecordPosition(leaf);
                    node = new AstBinop(TOKEN_DOT, node, leaf);
                    *(node->GetPositionRecord()) = pnfo;
                    Advance();
                }
                break;
            case TOKEN_ROUND_OPEN:
                node = new AstFunCall(node);
                RecordPosition(node);
                ParseArguments((AstFunCall*)node);
                UpdateEndPosition(node);
                break;
            default:
                done = true;
                break;
            }
        }
    } catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

AstIndexing *Parser::ParseRangesOrIndices(IAstExpNode *indexed)
{
    AstIndexing *node = NULL;
    AstIndexing *first;         // note: the first is at the bottom of the chain !
    
    try {
        while (m_token == TOKEN_SQUARE_OPEN) {
            do {
                if (node == NULL) {
                    first = node = new AstIndexing(indexed);
                } else {
                    node = new AstIndexing(node);
                }
                if (Advance() == TOKEN_COLON) {
                    RecordPosition(node);
                    Advance();
                    if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                        node->SetARange(NULL, ParseExpression());
                    } else {
                        node->SetARange(NULL, NULL);
                    }
                } else {
                    RecordPosition(node);
                    IAstExpNode *lower = ParseExpression();
                    if (m_token == TOKEN_COLON) {
                        Advance();
                        if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                            node->SetARange(lower, ParseExpression());
                        } else {
                            node->SetARange(lower, NULL);
                        }
                    } else {
                        node->SetAnIndex(lower);
                    }
                }
                UpdateEndPosition(node);
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                    Error("Expecting ']' or ','");
                }
            } while (m_token == TOKEN_COMMA);
            Advance();  // absorb ']'
        }
    } catch (...) {
        if (first != NULL) first->UnlinkIndexedTerm();  // it is deleted by the caller !
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

void Parser::ParseArguments(AstFunCall *node)
{
    AstArgument *argument;

    do {
        Advance();                      // absorb ( or ,
        if (m_token == TOKEN_COMMA) {
            node->AddAnArgument(NULL);
        } else if (m_token != TOKEN_ROUND_CLOSE) {
            argument = new AstArgument();
            node->AddAnArgument(argument);
            RecordPosition(argument);
            argument->SetExpression(ParseExpression());
            UpdateEndPosition(argument);
            if (m_token == TOKEN_COLON) {
                if (Advance() != TOKEN_NAME) {
                    Error("Expecting the parameter name");
                } 
                argument->AddName(m_lexer->CurrTokenString());
                UpdateEndPosition(argument);
                Advance();
            }
        }
        if (m_token != TOKEN_ROUND_CLOSE && m_token != TOKEN_COMMA) {
            Error("Expecting ')' or ','");
        }
    } while (m_token == TOKEN_COMMA);
    Advance();  // Absorb ')'
}

AstWhile *Parser::ParseWhile(void)
{
    AstWhile    *node = NULL;

    try {
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
        }
        Advance();
        node = new AstWhile();
        RecordPosition(node);
        node->SetExpression(ParseExpression());
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
        }
        Advance();
        UpdateEndPosition(node);
        node->SetBlock(ParseBlock());
    } catch (...) {
        if (node != NULL) delete node;
        throw;
    }
    return(node);
}

AstIf *Parser::ParseIf(void)
{
    AstIf *node = new AstIf();
    bool firstclause = true;
    RecordPosition(node);
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
            if (firstclause) {
                firstclause = false;
                UpdateEndPosition(node);
            }
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
    AstFor          *node = new AstFor();
    IAstExpNode     *first_expr;
    VarDeclaration  *var = NULL;

    try {
        RecordPosition(node);
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
        }
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the iterator or index name");
        }
        var = new VarDeclaration(m_lexer->CurrTokenString());
        RecordPosition(var);
        if (Advance() == TOKEN_COMMA) {
            if (Advance() != TOKEN_NAME) {
                Error("Expecting the iterator name");
            }
            var->SetFlags(VF_READONLY | VF_ISFORINDEX);
            node->SetIndexVar(var);
            var = NULL;                 // in case we have an exception in the next line
            var = new VarDeclaration(m_lexer->CurrTokenString());
            RecordPosition(var);
            var->SetFlags(VF_ISFORITERATOR);
            node->SetIteratorVar(var);
            var = NULL;                 // in case we have an exception in the next lines
            Advance();
        } else {
            var->SetFlags(VF_ISFORITERATOR);
            node->SetIteratorVar(var);
            var = NULL;
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
            node->iterator_->SetFlags(VF_READONLY);
        } else {
            node->SetTheSet(first_expr);
            node->iterator_->SetFlags(VF_IS_REFERENCE);
        }
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
        }
        Advance();
        UpdateEndPosition(node);
        node->SetBlock(ParseBlock());
    } catch (...) {
        delete node;
        if (var != NULL) delete var;
        throw;
    }
    return(node);
}

//
// switch '('  expression ')' '{' {single_case} [default_case] '}'
// single_case ::= expression ':' statement
// default_case ::= else ':' [statement]
//
AstSwitch *Parser::ParseSwitch(void)
{
    AstSwitch *node = new AstSwitch();
    try {
        RecordPosition(node);
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
        }
        Advance();  // absorb '('
        node->AddSwitchValue(ParseExpression());
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
        }
        if (Advance() != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
        }
        Advance();  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE && m_token != TOKEN_ELSE) {
            int recovery_level = curly_indent_;
            IAstExpNode* exp = nullptr;
            try {
                exp = ParseExpression();
                if (m_token != TOKEN_COLON) {
                    Error("Expecting ':'");
                }
                Advance(); // absorb ':'
                node->AddCase(exp, ParseStatement(false));
                exp = nullptr;
            }
            catch (...) {
                if (exp != nullptr) delete exp;
                if (!SkipToNextStatement(recovery_level)) {
                    throw;
                }
            }
        }
        if (m_token == TOKEN_ELSE) {
            if (Advance() != TOKEN_COLON) {
                Error("Expecting ':'");
            }
            if (Advance() == TOKEN_CURLY_CLOSE) {
                node->AddCase(nullptr, nullptr);
            } else {
                node->AddCase(nullptr, ParseStatement(false));
            }
            if (m_token != TOKEN_CURLY_CLOSE) {
                Error("Expecting '}', the else case must be the last of the switch !");
            }
        }
        Advance();  // absorb '}'
        UpdateEndPosition(node);
    }
    catch (...) {
        delete node;
        throw;
    }
    return(node);
}

//
// typeswitch '(' var_name '=' left_term '){' {single_type_case} [default_case] '}'
// single_type_case ::= class_type_name ':' statement
// default_case ::= else ':' [statement]
//
AstTypeSwitch *Parser::ParseTypeSwitch(void)
{
    AstTypeSwitch   *node = new AstTypeSwitch();
    VarDeclaration  *var = nullptr;
    try {
        RecordPosition(node);
        if (Advance() != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
        }
        if (Advance() != TOKEN_NAME) {
            Error("Expecting the reference name");
        }
        var = new VarDeclaration(m_lexer->CurrTokenString());
        RecordPosition(var);
        var->SetFlags(VF_IS_REFERENCE);
        if (Advance() != TOKEN_ASSIGN) {
            Error("Expecting '='");
        }
        Advance();  // absorb '='
        IAstExpNode *exp = ParseExpression();
        node->Init(var, exp);
        var = nullptr;
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
        }
        if (Advance() != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
        }
        Advance();  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE && m_token != TOKEN_ELSE) {
            int recovery_level = curly_indent_;
            AstNamedType *the_type = nullptr;
            try {
                the_type = ParseNamedType();
                if (m_token != TOKEN_COLON) {
                    Error("Expecting ':'");
                }
                Advance(); // absorb ':'
                node->AddCase(the_type, ParseStatement(false));
                the_type = nullptr;
            }
            catch (...) {
                if (the_type != nullptr) delete the_type;
                if (!SkipToNextStatement(recovery_level)) {
                    throw;
                }
            }
        }
        if (m_token == TOKEN_ELSE) {
            if (Advance() != TOKEN_COLON) {
                Error("Expecting ':'");
            }
            if (Advance() == TOKEN_CURLY_CLOSE) {
                node->AddCase(nullptr, nullptr);
            } else {
                node->AddCase(nullptr, ParseStatement(false));
                if (m_token != TOKEN_CURLY_CLOSE) {
                    Error("Expecting '}', the else case must be the last of the switch !");
                }
            }
        }
        Advance();  // absorb '}'
        UpdateEndPosition(node);
    }
    catch (...) {
        delete node;
        if (var != nullptr) delete var;
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
        if ((m_token == TOKEN_INLINE_COMMENT || m_token == TOKEN_EMPTY_LINES) && !for_reference_) {
            RemarkDescriptor *rd = new RemarkDescriptor;
            rd->row = m_lexer->CurrTokenLine();
            rd->col = m_lexer->CurrTokenColumn();
            if (m_token == TOKEN_INLINE_COMMENT) {
                rd->emptyline = false;
                rd->remark = m_lexer->CurrTokenVerbatim();
            } else {
                rd->emptyline = true;
            }
            root_->remarks_.push_back(rd);
        }
    } while (m_token == TOKEN_COMMENT || m_token == TOKEN_INLINE_COMMENT || m_token == TOKEN_EMPTY_LINES);
    if (m_token == TOKEN_CURLY_OPEN) {
        curly_indent_++;
    } else if (m_token == TOKEN_CURLY_CLOSE) {
        if (curly_indent_ > 0) {
            curly_indent_--;
        }
    }
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
    //char fullmessage[600];

    // errors happening while trying to recover are ignored.
    if (on_error_) {
        return;
    }
    errors_->AddError(message, row, column);
    //if (strlen(message) > 512) {
    //    errors_->AddName(message);
    //} else {
    //    sprintf(fullmessage, "line: %d \tcolumn: %d \t%s", row, column, message);
    //    errors_->AddName(fullmessage);
    //}
    has_errors_ = true;
    on_error_ = true;
}

bool Parser::SkipToNextStatement(int level)
{
    bool success = false;

    while (m_token != TOKEN_EOF) {
        if (curly_indent_ < level) break;
        if (curly_indent_ == level) {
            if (m_token == TOKEN_SEMICOLON) {
                SkipToken();
                success = true;
                break;
            } else if (m_token == TOKEN_CURLY_CLOSE) {
                SkipToken();
                if (m_token == TOKEN_ELSE || m_token == TOKEN_SEMICOLON) {
                    SkipToken();
                }
                success = true;
                break;
            }
        }
        SkipToken();
    }
    on_error_ = false;
    if (m_token == TOKEN_EOF) {
        return(false);
    }
    return(success);
}

void Parser::SkipToNextDeclaration(void)
{
    while (m_token != TOKEN_EOF && !(curly_indent_ == 0 && OnDeclarationToken())) {
        SkipToken();
    }
    on_error_ = false;
}

bool Parser::OnDeclarationToken(void) 
{
    switch (m_token) {
    case TOKEN_PUBLIC:
    case TOKEN_VAR:
    case TOKEN_LET:
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
    do {
        try {
            m_token = m_lexer->Advance();
        }
        catch (...) {
            // ignore errors while resyncing
        }
    } while (m_token == TOKEN_COMMENT || m_token == TOKEN_INLINE_COMMENT || m_token == TOKEN_EMPTY_LINES);
    if (m_token == TOKEN_CURLY_OPEN) {
        curly_indent_++;
    } else if (m_token == TOKEN_CURLY_CLOSE) {
        if (curly_indent_ > 0) {
            curly_indent_--;
        }
    }
    return(m_token);
}

void Parser::RecordPosition(IAstNode *node)
{
    if (node == NULL) return;
    FillPositionInfo(node->GetPositionRecord());
}

void Parser::FillPositionInfo(PositionInfo *pnfo)
{
    pnfo->start_row = m_lexer->CurrTokenLine();
    pnfo->end_row = m_lexer->CurrTokenLastLine();
    pnfo->start_col = m_lexer->CurrTokenColumn();
    pnfo->end_col = m_lexer->CurrTokenLastColumn();
    pnfo->last_row = pnfo->end_row;
    pnfo->last_col = pnfo->end_col;
}

void Parser::UpdateEndPosition(IAstNode *node)
{
    if (node == NULL) return;
    PositionInfo *pnfo = node->GetPositionRecord();
    pnfo->last_row = m_lexer->CurrTokenLastLine();
    pnfo->last_col = m_lexer->CurrTokenLastColumn();
}

void Parser::AttachCommentsToNodes(void)
{
    int remcount = root_->remarks_.size();
    if (remcount == 0) return;

    // collect eligible nodes
    vector<IAstNode*>   nodes;
    for (int ii = 0; ii < (int)root_->dependencies_.size(); ++ii) {
        nodes.push_back(root_->dependencies_[ii]);
    }
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        nodes.push_back(root_->declarations_[ii]);
    }

    // assign !!
    int currem = 0;
    int lastnode = nodes.size() - 1;
    for (int ii = 0; ii < lastnode && currem < remcount; ++ii) {
        IAstNode *node = nodes[ii];
        IAstNode *nextnode = nodes[ii + 1];
        int lastpos = node->GetPositionRecord()->last_row;
        int nextpos = nextnode->GetPositionRecord()->start_row;

        if (node->GetType() == ANT_FUNC) {
            FuncDeclaration *fun = (FuncDeclaration*)node;
            if (fun->block_ != nullptr) {
                lastpos = fun->block_->GetPositionRecord()->last_row;
            }
        }

        // first remark owned by the node
        int first = currem;             

        // advance to first remark past the node
        while (currem < remcount && root_->remarks_[currem]->row <= lastpos) {
            ++currem;
        }

        // the following remarks can be owned if: all the rows have a remark, there is a blank line separating this node from the next
        int temp = currem;
        while (temp < remcount) {
            RemarkDescriptor *rem = root_->remarks_[temp];
            if (rem->row >= nextpos) {

                // give up, there is no empty line before the next node.
                // remarks in between belong to the next node !
                break;
            }
            if (rem->emptyline) {

                // take the remark but not the empty line
                currem = temp;
                break;
            }
            ++temp;
        }

        // if has at least a remark, write it in the node !
        if (currem > first) {
            AssignCommentsToNode(node, first, currem - first);
        }
    }

    // the last node collects all the residual remarks
    if (currem < remcount) {
        AssignCommentsToNode(nodes[lastnode], currem, remcount - currem);
    }
}

void Parser::AssignCommentsToNode(IAstNode *node, int first, int count)
{
    PositionInfo *pos = node->GetPositionRecord();
    pos->first_remark = first;
    pos->num_remarks = count;
}

}