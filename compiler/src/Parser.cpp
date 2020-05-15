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
    ParseFile(root_, for_reference);
    if (on_error_) {
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

    if (!Advance()) return;
    if (m_token == TOKEN_NAMESPACE) {
        ParseNamespace(file);
        if (on_error_) {
            SkipToNextDeclaration();
        }
    }
    while (m_token == TOKEN_REQUIRES) {
        ParseDependency(file);
        if (on_error_) {
            SkipToNextDeclaration();
        }
    }
    while (m_token != TOKEN_EOF) {
        ParseDeclaration(file, for_reference);
        if (on_error_) {
            SkipToNextDeclaration();
        }
    }
}

void Parser::ParseNamespace(AstFile *file)
{
    if (!Advance()) return;
    if (m_token != TOKEN_NAME) {
        Error("Expecting the namespace name");
        return;
    } else {
        string n_space = m_lexer->CurrTokenString();
        if (!Advance()) return;
        while (m_token == TOKEN_DOT) {
            if (!Advance()) return;
            if (m_token != TOKEN_NAME) {
                Error("Expecting the next portion of a qualifyed name");
                return;
            }
            n_space += '.';
            n_space += m_lexer->CurrTokenString();
            if (!Advance()) return;
        }
        file->SetNamespace(n_space.c_str());
    }
    CheckSemicolon();
}

void Parser::ParseDependency(AstFile *file)
{
    if (!Advance()) return;
    if (m_token != TOKEN_LITERAL_STRING) {
        Error("Expecting the required package path (remember to enclose it in double quotes !)");
        return;
    } 
    AstDependency *dep = new AstDependency(m_lexer->CurrTokenString(), NULL);
    file->AddDependency(dep);
    RecordPosition(dep);
    if (!Advance()) return;
    if (m_token == TOKEN_COMMA) {
        if (!Advance()) return;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the required package local name");
            return;
        }
        dep->SetLocalPackageName(m_lexer->CurrTokenString());
        if (!Advance()) return;
    }
    UpdateEndPosition(dep);
    CheckSemicolon();
}

void Parser::ParseDeclaration(AstFile *file, bool for_reference)
{
    IAstDeclarationNode *node = NULL;
    bool    is_public = false;

    if (m_token == TOKEN_PUBLIC) {
        if (!Advance()) return;
        is_public = true;
    } else if (for_reference) {
        if (!Advance()) return;
        if (m_token == TOKEN_NAME) {
            file->AddPrivateSymbol(m_lexer->CurrTokenString());
        } else {
            Error("Expecting the declaration name");
            return;
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
        return;
    }
    if (!on_error_) {
        node->SetPublic(is_public);
        file->AddNode(node);
    }
}

VarDeclaration *Parser::ParseVar(void)
{
    VarDeclaration *var = NULL;
    bool            type_defined = false;

    if (!Advance()) return(nullptr);
    if (m_token != TOKEN_NAME) {
        Error("Expecting the variable name");
        return(nullptr);
    }
    var = new VarDeclaration(m_lexer->CurrTokenString());
    RecordPosition(var);
    {
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_ASSIGN && m_token != TOKEN_SEMICOLON) {
            var->SetType(ParseTypeSpecification());
            if (on_error_) goto recovery;
            type_defined = true;
        }
        if (m_token == TOKEN_ASSIGN) {
            if (!Advance()) goto recovery;
            var->SetIniter(ParseIniter());
            if (on_error_) goto recovery;
            type_defined = true;
        }
        if (!type_defined) {
            Error("You must either declare the variable type or use an initializer");
            goto recovery;
        }
        UpdateEndPosition(var);
        CheckSemicolon();
    }
recovery:
    if (on_error_) {
        delete var;
        return(nullptr);
    }
    return(var);
}

VarDeclaration *Parser::ParseConst(void)
{
    VarDeclaration *node;
    bool            type_defined = false;

    if (!Advance()) return(nullptr);
    if (m_token != TOKEN_NAME) {
        Error("Expecting the const name");
        return(nullptr);
    }
    node = new VarDeclaration(m_lexer->CurrTokenString());
    node->SetFlags(VF_READONLY);
    RecordPosition(node);
    {
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_ASSIGN) {
            node->SetType(ParseTypeSpecification());
            if (on_error_) goto recovery;
            type_defined = true;
        }
        if (m_token == TOKEN_ASSIGN) {
            if (!Advance()) goto recovery;
            node->SetIniter(ParseIniter());
            if (on_error_) goto recovery;
            type_defined = true;
        }
        if (!type_defined) {
            Error("You must either declare the const type or use an initializer");
            goto recovery;
        }
        UpdateEndPosition(node);
        CheckSemicolon();
    } 
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

TypeDeclaration *Parser::ParseType(void)
{
    TypeDeclaration *node;

    if (!Advance()) return(nullptr);
    if (m_token != TOKEN_NAME) {
        Error("Expecting the type name");
        return(nullptr);
    }
    node = new TypeDeclaration(m_lexer->CurrTokenString());
    RecordPosition(node);
    {
        if (!Advance()) goto recovery;
        node->SetType(ParseTypeSpecification());
        if (on_error_) goto recovery;
        if (m_token == TOKEN_ASSIGN) {
            Error("Can't initialize a type !");
            goto recovery;
        }
        CheckSemicolon();
    } 
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

//func_definition ::= func func_fullname function_type block
FuncDeclaration *Parser::ParseFunctionDeclaration(bool skip_body)
{
    string          name1, name2;
    bool            is_member = false;
    FuncDeclaration *node = NULL;

    {
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the function name");
            goto recovery;
        }
        name1 = m_lexer->CurrTokenString();
        node = new FuncDeclaration();
        RecordPosition(node);
        if (!Advance()) goto recovery;
        if (m_token == TOKEN_DOT) {
            if (!Advance()) goto recovery;
            is_member = true;
            if (m_token == TOKEN_NAME) {
                name2 = m_lexer->CurrTokenString();
                if (!Advance()) goto recovery;
            } else {
                Error("Expecting a member function name after the member selector '.'");
                goto recovery;
            }
        }
        node->SetNames(name1.c_str(), name2.c_str());
        AstFuncType *ftype = ParseFunctionType(true);
        node->AddType(ftype);
        if (on_error_) goto recovery;
        if (is_member) ftype->SetIsMember();
        UpdateEndPosition(node);
        if (skip_body) {
            SkipToNextDeclaration();
        } else {
            node->AddBlock(ParseBlock());
        }
    } 
recovery:    
    if (on_error_) {
        if (node != nullptr) delete node;
        node = nullptr;
    }
    return(node);
}

//
// enum_decl :: = enum <type_name> ' { '(enum_element) ' }'
// enum_element :: = <label_name>['=' const_expression]
//
TypeDeclaration *Parser::ParseEnum(void)
{
    TypeDeclaration *node = nullptr;
    {
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the type name");
            goto recovery;
        }
        node = new TypeDeclaration(m_lexer->CurrTokenString());
        AstEnumType *typenode = new AstEnumType();
        node->SetType(typenode);
        RecordPosition(node);
        RecordPosition(typenode);
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
            goto recovery;
        }
        do {
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_NAME) {
                Error("Expecting an enum case name");
                goto recovery;
            }
            string the_case = m_lexer->CurrTokenString();
            if (!Advance()) goto recovery;
            if (m_token == TOKEN_ASSIGN) {
                if (!Advance()) goto recovery;  // absorb '='
                typenode->AddItem(the_case.c_str(), ParseExpression());
                if (on_error_) goto recovery;
            } else {
                typenode->AddItem(the_case.c_str(), nullptr);
            }
        } while (m_token == TOKEN_COMMA);
        if (m_token != TOKEN_CURLY_CLOSE) {
            Error("Expecting '}'");
            goto recovery;
        }
        UpdateEndPosition(node);
        Advance();  // absorb '}'
    }
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

//
//interface_decl :: = interface<if_name> '{ '(member_function_declaration | interface <interface_name> ';') ' }'
//
TypeDeclaration *Parser::ParseInterface(void)
{
    TypeDeclaration *node = nullptr;
    {
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the type name");
            goto recovery;
        }
        node = new TypeDeclaration(m_lexer->CurrTokenString());
        AstInterfaceType *typenode = new AstInterfaceType();
        node->SetType(typenode);
        RecordPosition(node);
        RecordPosition(typenode);
        if (!Advance()) goto recovery;

        if (m_token == TOKEN_COLON) {
            do {
                if (!Advance()) goto recovery; // absorb ':' or ','
                typenode->AddAncestor(ParseNamedType());
                if (on_error_) goto recovery;
            } while (m_token == TOKEN_COMMA);
        }

        if (m_token != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
            goto recovery;
        }
        if (!Advance()) goto recovery;  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE) {
            int recovery_level = curly_indent_;
            FuncDeclaration *fun = nullptr;
            {
                if (m_token == TOKEN_FUNC) {
                    bool is_mutable = false;
                    if (!Advance()) goto recovery2;
                    if (m_token == TOKEN_MUT) {
                        is_mutable = true;
                        if (!Advance()) goto recovery2;
                    } 
                    if (m_token != TOKEN_NAME) {
                        Error("Expecting the function name");
                        goto recovery2;
                    }
                    fun = new FuncDeclaration();
                    RecordPosition(fun);
                    fun->SetNames(m_lexer->CurrTokenString(), "");
                    fun->SetMuting(is_mutable);
                    fun->SetPublic(true);
                    if (!Advance()) goto recovery2;
                    AstFuncType *ftype = ParseFunctionType(false);
                    fun->AddType(ftype);
                    if (on_error_) goto recovery;
                    ftype->SetIsMember();
                    UpdateEndPosition(fun);
                    typenode->AddMember(fun);
                    fun = nullptr;
                    CheckSemicolon();
                // } else if (m_token == TOKEN_INTERFACE) {
                //     if (!Advance()) goto recovery2;
                //     typenode->AddAncestor(ParseNamedType());
                //     if (on_error_) goto recovery2;
                //     CheckSemicolon();
                } else {
                    Error("Expecting 'fn' or 'interface'. Note: public/private qualifiers are not allowed here.");
                    goto recovery2;
                }
            }
recovery2:    
            if (on_error_) {
                if (fun != nullptr) delete fun;
                if (!SkipToNextStatement(recovery_level)) {
                    goto recovery;
                }
            }
        };
        UpdateEndPosition(node);
        Advance();  // absorb '}'
    }
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

//
// class_decl :: = class <type_name> '{ '(class_section) ' }'
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
    AstNamedType *named = nullptr;
    bool public_section = false;
    {
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the type name");
            goto recovery2;
        }
        node = new TypeDeclaration(m_lexer->CurrTokenString());
        AstClassType *typenode = new AstClassType();
        node->SetType(typenode);
        RecordPosition(node);
        RecordPosition(typenode);
        if (!Advance()) goto recovery2;

        if (m_token == TOKEN_COLON) {
            do {
                if (!Advance()) goto recovery2; // absorb ':' or ','

                named = ParseNamedType();
                if (on_error_) goto recovery2;
                if (m_token == TOKEN_BY) {
                    if (!Advance()) goto recovery2;
                    if (m_token != TOKEN_NAME) {
                        Error("Expecting the name of the var member implementing the interface");
                        goto recovery2;
                    }
                    typenode->AddMemberInterface(named, m_lexer->CurrTokenString());
                    named = nullptr;
                    if (!Advance()) goto recovery2;
                } else {
                    typenode->AddMemberInterface(named, "");
                    named = nullptr;
                }
            } while (m_token == TOKEN_COMMA);
        }

        

        if (m_token != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE) {
            int recovery_level = curly_indent_;
            FuncDeclaration *fun = nullptr;
            //AstNamedType *named = nullptr;
            {
                if (m_token == TOKEN_PRIVATE) {
                    public_section = false;
                    if (!Advance()) goto recovery;
                    if (m_token != TOKEN_COLON) {
                        Error("Expecting ':'");
                        goto recovery;
                    }
                    if (!Advance()) goto recovery;;
                } else if (m_token == TOKEN_PUBLIC) {
                    public_section = true;
                    if (!Advance()) goto recovery;
                    if (m_token != TOKEN_COLON) {
                        Error("Expecting ':'");
                        goto recovery;
                    }
                    if (!Advance()) goto recovery;;
                } else if (m_token == TOKEN_VAR) {
                    VarDeclaration *var = ParseVar();
                    if (on_error_) goto recovery;
                    var->SetPublic(public_section);
                    typenode->AddMemberVar(var);
                } else if (m_token == TOKEN_FUNC) {
                    bool is_mutable = false;
                    if (!Advance()) goto recovery;
                    if (m_token == TOKEN_MUT) {
                        is_mutable = true;
                        if (!Advance()) goto recovery;
                    } 
                    if (m_token != TOKEN_NAME) {
                        Error("Expecting the function name");
                        goto recovery;
                    }
                    fun = new FuncDeclaration();
                    RecordPosition(fun);
                    fun->SetNames(m_lexer->CurrTokenString(), "");
                    fun->SetMuting(is_mutable);
                    fun->SetPublic(public_section);
                    if (!Advance()) goto recovery;;
                    if (m_token == TOKEN_BY) {
                        if (!public_section) {
                            Error("You can't delegate a private function");
                            goto recovery;
                        }
                        if (!Advance()) goto recovery;
                        if (m_token != TOKEN_NAME) {
                            Error("Expecting the name of the var member implementing the function");
                            goto recovery;
                        }
                        UpdateEndPosition(fun);
                        typenode->AddMemberFun(fun, m_lexer->CurrTokenString());
                        fun = nullptr;
                        if (!Advance()) goto recovery;
                    } else {
                        AstFuncType *ftype = ParseFunctionType(false);
                        fun->AddType(ftype);
                        if (on_error_) goto recovery;
                        ftype->SetIsMember();
                        UpdateEndPosition(fun);
                        typenode->AddMemberFun(fun, "");
                        fun = nullptr;
                    }
                    CheckSemicolon();
                // } else if (m_token == TOKEN_INTERFACE) {
                //     if (!public_section) {
                //         Error("All the interfaces must be public");
                //         goto recovery;
                //     }
                //     if (!Advance()) goto recovery;
                //     named = ParseNamedType();
                //     if (on_error_) goto recovery;
                //     if (m_token == TOKEN_BY) {
                //         if (!Advance()) goto recovery;
                //         if (m_token != TOKEN_NAME) {
                //             Error("Expecting the name of the var member implementing the interface");
                //             goto recovery;
                //         }
                //         typenode->AddMemberInterface(named, m_lexer->CurrTokenString());
                //         named = nullptr;
                //         if (!Advance()) goto recovery;
                //     } else {
                //         typenode->AddMemberInterface(named, "");
                //         named = nullptr;
                //     }
                //     CheckSemicolon();
                } else {
                    Error("Expecting a var/fn/interface declaration or a public/private qualifier");
                    goto recovery;
                }
            }
recovery:            
            if (on_error_) {
                if (fun != nullptr) delete fun;
                //if (named != nullptr) delete named;
                if (!SkipToNextStatement(recovery_level)) {
                    goto recovery2;
                }
            }
        }
        UpdateEndPosition(node);
        Advance();  // absorb '}'
    }
recovery2:
    if (on_error_) {
        delete node;
        node = nullptr;
        if (named != nullptr) delete named;
    }
    return(node);
}

/*
type_specification ::= base_type | <type_name> | <pkg_name>.<type_name> |
                        map '(' type_specification ')' type_specification |
                        {'[' ([const_expression]) ']'} type_specification |
                        matrix {'[' ([ expression[..expression] ]) ']'} type_specification |
                        [const] [weak] '*' type_specification |
                        function_type
*/
IAstTypeNode *Parser::ParseTypeSpecification(void)
{
    IAstTypeNode *node = NULL;

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
        case TOKEN_BOOL:
        case TOKEN_ERRORCODE:
        case TOKEN_VOID:
            node = new AstBaseType(m_token);
            RecordPosition(node);
            if (!Advance()) goto recovery;
            break;
        case TOKEN_NAME:
            node = ParseNamedType();
            break;
        case TOKEN_MAP:
            {
                AstMapType *map = new AstMapType();
                node = map;
                RecordPosition(node);
                if (!Advance()) goto recovery;
                if (m_token != TOKEN_ROUND_OPEN) {
                    Error("Expecting '('");
                    goto recovery;
                }
                if (!Advance()) goto recovery;
                map->SetKeyType(ParseTypeSpecification());
                if (on_error_) goto recovery;
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')'");
                    goto recovery;
                }
                if (!Advance()) goto recovery;
                map->SetReturnType(ParseTypeSpecification());
                if (on_error_) goto recovery;
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
                    if (!Advance()) goto recovery;
                }
                if (m_token != TOKEN_MPY) {
                    Error("Expecting '*'");
                    goto recovery;
                }
                node = new AstPointerType();
                RecordPosition(node);
                if (!Advance()) goto recovery;
                ((AstPointerType*)node)->Set(isconst, isweak, ParseTypeSpecification());
            }
            break;
        case TOKEN_ROUND_OPEN:
        case TOKEN_PURE:
            node = ParseFunctionType(false);
            break;
        default:
            Error("Invalid type declaration");
            goto recovery;
        }
    } 
recovery:    
    if (on_error_) {
        if (node != NULL) delete node;
        node = nullptr;
    }
    return(node);
}

AstNamedType *Parser::ParseNamedType(void)
{
    AstNamedType *node = new AstNamedType(m_lexer->CurrTokenString());
    {
        if (m_token != TOKEN_NAME) {
            Error("Expecting a name");
            if (on_error_) goto recovery;
        }
        AstNamedType *last = node;
        RecordPosition(node);
        if (!Advance()) goto recovery;
        while (m_token == TOKEN_DOT) {
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_NAME) {
                Error("Expecting the next portion of a qualifyed name");
                if (on_error_) goto recovery;
            }
            AstNamedType *curr = new AstNamedType(m_lexer->CurrTokenString());
            RecordPosition(curr);
            last->ChainComponent(curr);
            last = curr;
            if (!Advance()) goto recovery;
        }
    }
recovery:    
    if (on_error_) {
        if (node != nullptr) delete node;
        node = nullptr;
    }
    return(node);
}

AstArrayType *Parser::ParseIndices(void)
{
    AstArrayType    *root = NULL;
    AstArrayType    *last, *curr;

    {
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
                if (!Advance()) goto recovery;
                RecordPosition(curr);
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA && m_token != TOKEN_MPY) {
                    curr->SetDimensionExpression(ParseExpression());
                    if (on_error_) goto recovery;
                } else if (m_token == TOKEN_MPY) {
                    curr->SetDynamic(true);
                    if (!Advance()) goto recovery;
                    curr->SetRegular(m_token == TOKEN_COMMA);
                }
                UpdateEndPosition(curr);
                if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                    Error("Expecting ']' or ','");
                    goto recovery;
                }
            } while (m_token == TOKEN_COMMA);
            if (!Advance()) goto recovery;  // absorb ']'
        }
        last->SetElementType(ParseTypeSpecification());
    } 
recovery:    
    if (on_error_) {
        if (root != nullptr) delete root;
        root = nullptr;
    }
    return(root);
}

// initer :: = expression | '{ '(initer) ' }'
IAstNode *Parser::ParseIniter(void)
{
    if (m_token == TOKEN_CURLY_OPEN) {
        AstIniter *node = new AstIniter();
        RecordPosition(node);
        {
            do {
                if (!Advance()) goto recovery;
                node->AddElement(ParseIniter());
                if (on_error_) goto recovery;
            } while (m_token == TOKEN_COMMA);
            if (m_token != TOKEN_CURLY_CLOSE) {
                Error("Expecting '}'");
                goto recovery;
            }
            UpdateEndPosition(node);
            if (!Advance()) goto recovery;  // absorb }
        } 
recovery:    
        if (on_error_) {
            delete node;
            node = nullptr;
        }
        return(node);
    }
    return(ParseExpression());
}

//function_type ::= [pure] argsdef type_specification
AstFuncType *Parser::ParseFunctionType(bool is_body)
{
    AstFuncType *node = NULL;

    {
        if (m_token == TOKEN_PURE) {
            node = new AstFuncType(true);
            if (!Advance()) goto recovery;
        } else {
            node = new AstFuncType(false);
        }
        RecordPosition(node);
        ParseArgsDef(node, is_body);
        if (on_error_) goto recovery;
        node->SetReturnType(ParseTypeSpecification());
    } 
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

// argsdef ::=  '(' ( single_argdef ) [',' ...] ')' | '(' ' ')'
void Parser::ParseArgsDef(AstFuncType *desc, bool is_function_body)
{
    if (m_token != TOKEN_ROUND_OPEN) {
        Error("Expecting '('");
        return;
    }
    if (!Advance()) return;
    if (m_token != TOKEN_ROUND_CLOSE) {

        // used only inside AddArgument to check if all the args after the first inited one have initers.
        bool mandatory_initer = false;

        while (true) {
            if (m_token == TOKEN_ETC) {
                desc->SetVarArgs();
                if (!Advance()) return;
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')' no other arguments allowed after ellipsis");
                    return;
                }
                break;
            } else {
                desc->AddArgument(ParseSingleArgDef(is_function_body, &mandatory_initer));
                if (on_error_) return;
            }
            if (m_token == TOKEN_ROUND_CLOSE) {
                break;
            } else if (m_token != TOKEN_COMMA) {
                Error("Expecting ','");
                return;
            }
            if (!Advance()) return;  // absorb ','
        }
    }
    if (!Advance()) return;  // absorb ')'
}

//single_argdef :: = [arg_direction] <arg_name> type_specification[' = ' initer]
// arg_direction :: = out | io | in
VarDeclaration *Parser::ParseSingleArgDef(bool is_function_body, bool *mandatory_initer)
{
    Token direction = m_token;
    VarDeclaration *node;

    switch (m_token) {
    case TOKEN_IN:
    case TOKEN_OUT:
    case TOKEN_IO:
        if (!Advance()) return(nullptr);
        break;
    default:
        direction = TOKEN_IN;
        break;
    }
    if (m_token != TOKEN_NAME) {
        Error("Expecting the argument name");
        return(nullptr);
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
    {
        if (!Advance()) goto recovery;
        node->SetType(ParseTypeSpecification());
        if (on_error_) goto recovery;
        if (m_token == TOKEN_ASSIGN) {
            if (!Advance()) goto recovery;
            node->SetIniter(ParseIniter());
            if (on_error_) goto recovery;
            *mandatory_initer = true;
        } else if (*mandatory_initer) {
            Error("All arguments following a default arg must have a default value.");
        }
        UpdateEndPosition(node);
    } 
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

//block :: = '{ '{ block_item } ' }'
//block_item :: = var_decl | const_decl | statement | block
/* statements:
( left_term ) '=' ( expression ) |
left_term update_operator expression |
left_term ++ | left_term -- |
functioncall |
while '(' expression ')' block |
if '(' expression ')' block {[ else if '(' expression ')' block ]} [else block] |
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
        return(nullptr);
    }
    AstBlock *node = new AstBlock();
    RecordPosition(node);
    {
        if (!Advance()) goto recovery;
        while (m_token != TOKEN_CURLY_CLOSE) {
            IAstNode *statement = ParseStatement(true);
            if (on_error_) goto recovery;
            if (statement != nullptr) {
                node->AddItem(statement);
            }
        }
        UpdateEndPosition(node);
        Advance();
    } 
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
    }
    return(node);
}

IAstNode *Parser::ParseStatement(bool allow_let_and_var)
{
    IAstNode *node = nullptr;
    int recovery_level = curly_indent_;
    {
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
            if (!Advance()) goto recovery;
            CheckSemicolon();
        }
        break;
        case TOKEN_RETURN:
        {
            AstReturn *ast_ret = new AstReturn();
            node = ast_ret;
            RecordPosition(ast_ret);
            if (!Advance()) goto recovery;
            if (m_token == TOKEN_ROUND_OPEN) {
                if (!Advance()) goto recovery;
                ast_ret->AddRetExp(ParseExpression());
                if (on_error_) goto recovery;
                if (m_token != TOKEN_ROUND_CLOSE) {
                    Error("Expecting ')'");
                    goto recovery;
                }
                if (!Advance()) goto recovery;
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
            if (!Advance()) goto recovery;
            aid->SetLeftTerm(ParseLeftTerm());
            if (on_error_) goto recovery;
            CheckSemicolon();
        }
        break;
        default:
            node = ParseLeftTermStatement();
            break;
        }
    } 
recovery:    
    if (on_error_) {
        if (node != nullptr) {
            delete node;
            node = nullptr;
        }
        SkipToNextStatement(recovery_level);
    }
    return(node);
}

//left_term = expression |
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

    {
        assignee = ParseLeftTerm("Expecting a statement");
        if (on_error_) goto recovery;
        FillPositionInfo(&pinfo);   // start with token
        token = m_token;
        switch (m_token) {
        case TOKEN_ASSIGN:
        case TOKEN_UPD_PLUS:
        case TOKEN_UPD_MINUS:
        case TOKEN_UPD_MPY:
        case TOKEN_UPD_DIVIDE:
        case TOKEN_UPD_XOR:
        case TOKEN_UPD_MOD:
        case TOKEN_UPD_SHR:
        case TOKEN_UPD_SHL:
        case TOKEN_UPD_AND:
        case TOKEN_UPD_OR:
            if (!Advance()) goto recovery;
            node = new AstUpdate(token, assignee, ParseExpression());
            assignee = nullptr;
            if (on_error_) goto recovery;
            *(node->GetPositionRecord()) = pinfo;
            break;
        case TOKEN_INC:
        case TOKEN_DEC:
            node = new AstIncDec(token);
            ((AstIncDec*)node)->SetLeftTerm(assignee);
            *(node->GetPositionRecord()) = pinfo;
            assignee = NULL;
            if (!Advance()) goto recovery;
            break;
        default:
            if (assignee->GetType() != ANT_FUNCALL) {
                Error("Expression or part of it has no effects");
                goto recovery;
            }
            node = assignee;
            assignee = NULL;
            break;      // let's assume it is a function call.
        }
        UpdateEndPosition(node);
        CheckSemicolon();
    } 
recovery:    
    if (on_error_) {
        if (node != nullptr) delete node;
        if (assignee != nullptr) delete assignee;
        node = nullptr;
    }
    return(node);
}

/*
expression :: = prefix_expression | expression binop expression

binop ::=  '+' | '-' | '*' | '/' | '^' | '%' | 
	  '&' | '|' | '>>' | '<<' |
	  '<' | '<=' | '>' | '>=' | '==' | '!=' | 
	  '**' | && | || 
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

    {
        do {
            nodes[num_nodes++] = ParsePrefixExpression();
            if (on_error_) goto recovery;
            m_lexer->ConvertToPower(&m_token);              // if appropriate convert to the power operator
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
                if (!Advance()) goto recovery;
            }
        } while (priority <= Lexer::max_priority);
    } 
recovery:    
    if (on_error_) {
        for (int ii = 0; ii < num_nodes; ++ii) {
            delete nodes[ii];
        }
        return(nullptr);
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
//          unop prefix_expression | '(' expression ')'
//
// unop :: = ' - ' | '!' | '~' | '&'
//
IAstExpNode *Parser::ParsePrefixExpression(void)
{
    IAstExpNode    *node = NULL;

    {
        switch (m_token) {
        case TOKEN_NULL:
        case TOKEN_FALSE:
        case TOKEN_TRUE:
            node = new AstExpressionLeaf(m_token, "");
            RecordPosition(node);
            if (!Advance()) goto recovery;
            break;
        case TOKEN_LITERAL_STRING:
            node = new AstExpressionLeaf(m_token, m_lexer->CurrTokenVerbatim());
            RecordPosition(node);
            if (!Advance()) goto recovery;
            while (m_token == TOKEN_LITERAL_STRING)
            {
                ((AstExpressionLeaf*)node)->AppendToValue("\xff");
                ((AstExpressionLeaf*)node)->AppendToValue(m_lexer->CurrTokenVerbatim());
                if (!Advance()) goto recovery;
            };
            break;
        case TOKEN_LITERAL_UINT:
        case TOKEN_LITERAL_FLOAT:
        case TOKEN_LITERAL_IMG:
            node = new AstExpressionLeaf(m_token, m_lexer->CurrTokenVerbatim());
            RecordPosition(node);
            if (!Advance()) goto recovery;
            break;
        case TOKEN_SIZEOF:
            node = new AstUnop(TOKEN_SIZEOF);
            RecordPosition(node);
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            if (m_token == TOKEN_NAME || m_token == TOKEN_MPY) {
                ((AstUnop*)node)->SetOperand(ParseLeftTerm());
            } else {
                ((AstUnop*)node)->SetTypeOperand(ParseTypeSpecification());
            }
            if (on_error_) goto recovery;
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            break;
        case TOKEN_DIMOF:
            node = new AstUnop(TOKEN_DIMOF);
            RecordPosition(node);
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            ((AstUnop*)node)->SetOperand(ParseLeftTerm());
            if (on_error_) goto recovery;
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
                goto recovery;
            }
            if (!Advance()) goto recovery;
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
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            ((AstUnop*)node)->SetOperand(ParseExpression());
            if (on_error_) goto recovery;
            node = CheckForCastedLiterals((AstUnop*)node);
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            break;
        // case TOKEN_ROUND_OPEN:
        //     if (!Advance()) goto recovery;
        //     node = ParseExpression();
        //     if (on_error_) goto recovery;
        //     if (m_token != TOKEN_ROUND_CLOSE) {
        //         Error("Expecting ')'");
        //         goto recovery;
        //     }
        //     if (!Advance()) goto recovery;
        //     break;
        case TOKEN_MINUS:
        case TOKEN_PLUS:
        case TOKEN_AND:
        case TOKEN_NOT:
        case TOKEN_LOGICAL_NOT:
        //case TOKEN_DOT:
            node = new AstUnop(m_token);
            RecordPosition(node);
            if (!Advance()) goto recovery;
            ((AstUnop*)node)->SetOperand(ParsePrefixExpression());
            break;
        default:
            node = ParseLeftTerm("Expecting an expression");
            break;
        }
    } 
recovery:    
    if (on_error_) {
        if (node != NULL) delete node;
        node = nullptr;
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

//left_term :: = <var_name> | '(' left_term ')' | '*'left_term
//               left_term '[' indices_or_rages ']' | left_term '.' <name> | left_term '(' arguments ')' | this
IAstExpNode *Parser::ParseLeftTerm(const char *errmess)
{
    IAstExpNode    *node = NULL;

    {
        switch (m_token) {
        case TOKEN_MPY:
            node = new AstUnop(TOKEN_MPY);
            RecordPosition(node);
            if (!Advance()) goto recovery;
            ((AstUnop*)node)->SetOperand(ParseLeftTerm());
            if (on_error_) goto recovery;
            break;
        case TOKEN_ROUND_OPEN:
            if (!Advance()) goto recovery;
            node = ParseExpression();       // es: (...).sqrt()
            //node = ParseLeftTerm();
            if (on_error_) goto recovery;
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
                goto recovery;                
            }
            if (!Advance()) goto recovery;  // absorb ')'
            break;
        case TOKEN_NAME:
            node = new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString());
            RecordPosition(node);
            if (!Advance()) goto recovery;
            break;
        case TOKEN_THIS:
            node = new AstExpressionLeaf(TOKEN_THIS, "");
            RecordPosition(node);
            if (!Advance()) goto recovery;
            break;
        default:
            Error(errmess != NULL ? errmess : "Expecting a left term (an assignable expression)");
            goto recovery;                
        }

        // postfixed ?
        bool done = false;
        while (!done) {
            switch (m_token) {
            case TOKEN_SQUARE_OPEN:
                node = ParseRangesOrIndices(node);
                if (on_error_) goto recovery;
                break;
            case TOKEN_DOT:
                {
                    PositionInfo pnfo;
                    FillPositionInfo(&pnfo);
                    if (!Advance()) goto recovery;
                    if (m_token != TOKEN_NAME) {
                        Error("Expecting a field name or a symbol");
                        goto recovery;                
                    }
                    AstExpressionLeaf *leaf = new AstExpressionLeaf(TOKEN_NAME, m_lexer->CurrTokenString());
                    RecordPosition(leaf);
                    node = new AstBinop(TOKEN_DOT, node, leaf);
                    *(node->GetPositionRecord()) = pnfo;
                    if (!Advance()) goto recovery;
                }
                break;
            case TOKEN_ROUND_OPEN:
                node = new AstFunCall(node);
                RecordPosition(node);
                ParseArguments((AstFunCall*)node);
                if (on_error_) goto recovery;                
                UpdateEndPosition(node);
                break;
            default:
                done = true;
                break;
            }
        }
    } 
recovery:    
    if (on_error_) {
        if (node != NULL) delete node;
        node = nullptr;
    }
    return(node);
}

AstIndexing *Parser::ParseRangesOrIndices(IAstExpNode *indexed)
{
    AstIndexing *node = NULL;
    AstIndexing *first;         // note: the first is at the bottom of the chain !
    
    {
        while (m_token == TOKEN_SQUARE_OPEN) {
            do {
                if (node == NULL) {
                    first = node = new AstIndexing(indexed);
                } else {
                    node = new AstIndexing(node);
                }
                if (!Advance()) goto recovery;
                if (m_token == TOKEN_COLON) {
                    RecordPosition(node);
                    if (!Advance()) goto recovery;
                    if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                        node->SetARange(NULL, ParseExpression());
                        if (on_error_) goto recovery;
                    } else {
                        node->SetARange(NULL, NULL);
                    }
                } else {
                    RecordPosition(node);
                    IAstExpNode *lower = ParseExpression();
                    if (on_error_) goto recovery;
                    if (m_token == TOKEN_COLON) {
                        if (!Advance()) goto recovery;
                        if (m_token != TOKEN_SQUARE_CLOSE && m_token != TOKEN_COMMA) {
                            node->SetARange(lower, ParseExpression());
                            if (on_error_) goto recovery;
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
                    goto recovery;
                }
            } while (m_token == TOKEN_COMMA);
            if (!Advance()) goto recovery;  // absorb ']'
        }
    } 
recovery:    
    if (on_error_) {
        if (first != nullptr) first->UnlinkIndexedTerm();  // it is deleted by the caller !
        if (node != nullptr) delete node;
        node = nullptr;
    }
    return(node);
}

void Parser::ParseArguments(AstFunCall *node)
{
    AstArgument *argument;

    do {
        if (!Advance()) return; // absorb ( or ,
        if (m_token == TOKEN_COMMA) {
            node->AddAnArgument(NULL);
        } else if (m_token != TOKEN_ROUND_CLOSE) {
            argument = new AstArgument();
            node->AddAnArgument(argument);
            RecordPosition(argument);
            argument->SetExpression(ParseExpression());
            if (on_error_) return;
            UpdateEndPosition(argument);
            if (m_token == TOKEN_COLON) {
                if (!Advance()) return;
                if (m_token != TOKEN_NAME) {
                    Error("Expecting the parameter name");
                    return;
                } 
                argument->AddName(m_lexer->CurrTokenString());
                UpdateEndPosition(argument);
                if (!Advance()) return;;
            }
        }
        if (m_token != TOKEN_ROUND_CLOSE && m_token != TOKEN_COMMA) {
            Error("Expecting ')' or ','");
            return;
        }
    } while (m_token == TOKEN_COMMA);
    Advance();  // Absorb ')'
}

AstWhile *Parser::ParseWhile(void)
{
    AstWhile    *node = NULL;

    if (!Advance()) return(nullptr);
    if (m_token != TOKEN_ROUND_OPEN) {
        Error("Expecting '('");
        return(nullptr);
    }
    if (!Advance()) return(nullptr);
    {
        node = new AstWhile();
        RecordPosition(node);
        node->SetExpression(ParseExpression());
        if (on_error_) goto recovery;
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
            goto recovery;
        }
        if (!Advance()) goto recovery;
        UpdateEndPosition(node);
        node->SetBlock(ParseBlock());
    }
recovery:    
    if (on_error_) {
        if (node != NULL) delete node;
        node = nullptr;
    }
    return(node);
}

AstIf *Parser::ParseIf(void)
{
    AstIf *node = new AstIf();
    bool firstclause = true;
    RecordPosition(node);
    {
        do {
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_ROUND_OPEN) {
                Error("Expecting '('");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            node->AddExpression(ParseExpression());
            if (on_error_) goto recovery;
            if (m_token != TOKEN_ROUND_CLOSE) {
                Error("Expecting ')'");
                goto recovery;
            }
            if (!Advance()) goto recovery;
            if (firstclause) {
                firstclause = false;
                UpdateEndPosition(node);
            }
            node->AddBlock(ParseBlock());
            if (on_error_) goto recovery;
            if (m_token != TOKEN_ELSE) break;   // done !
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_IF) {
                node->SetDefaultBlock(ParseBlock());
                if (on_error_) goto recovery;
                break;
            }
        } while (true);
    } 
recovery:    
    if (on_error_) {
        delete node;
        node = nullptr;
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

    {
        RecordPosition(node);
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
            goto recovery;
        }
        if (!Advance()) goto recovery;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the iterator or index name");
            goto recovery;
        }
        var = new VarDeclaration(m_lexer->CurrTokenString());
        RecordPosition(var);
        if (!Advance()) goto recovery;
        if (m_token == TOKEN_COMMA) {
            if (!Advance()) goto recovery;
            if (m_token != TOKEN_NAME) {
                Error("Expecting the iterator name");
                goto recovery;
            }
            var->SetFlags(VF_READONLY | VF_ISFORINDEX);
            node->SetIndexVar(var);
            var = NULL;                 // in case we have an exception in the next line
            var = new VarDeclaration(m_lexer->CurrTokenString());
            RecordPosition(var);
            var->SetFlags(VF_ISFORITERATOR);
            node->SetIteratorVar(var);
            var = NULL;                 // in case we have an exception in the next lines
            if (!Advance()) goto recovery;
        } else {
            var->SetFlags(VF_ISFORITERATOR);
            node->SetIteratorVar(var);
            var = NULL;
        }
        if (m_token != TOKEN_IN) {
            Error("Expecting 'in' followed by the for iteration range");
            goto recovery;
        }
        if (!Advance()) goto recovery;
        first_expr = ParseExpression();
        if (on_error_) goto recovery;
        if (m_token == TOKEN_COLON) {
            node->SetLowBound(first_expr);
            if (!Advance()) goto recovery;
            node->SetHightBound(ParseExpression());
            if (on_error_) goto recovery;
            if (m_token == TOKEN_STEP) {
                if (!Advance()) goto recovery;
                node->SetStep(ParseExpression());
                if (on_error_) goto recovery;
            }
            node->iterator_->SetFlags(VF_READONLY);
        } else {
            node->SetTheSet(first_expr);
            node->iterator_->SetFlags(VF_IS_REFERENCE);
        }
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
            goto recovery;
        }
        if (!Advance()) goto recovery;
        UpdateEndPosition(node);
        node->SetBlock(ParseBlock());
    } 
recovery:    
    if (on_error_) {
        delete node;
        if (var != NULL) delete var;
        node = nullptr;
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
    {
        RecordPosition(node);
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;  // absorb '('
        node->AddSwitchValue(ParseExpression());
        if (on_error_) goto recovery2;
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE && m_token != TOKEN_DEFAULT) {
            int recovery_level = curly_indent_;
            do {
                if (m_token != TOKEN_CASE) {
                    Error("Expecting 'case'");
                    goto recovery;
                }
                if (!Advance()) goto recovery; // absorb 'case'
                node->AddCase(ParseExpression());
                if (on_error_) goto recovery;
                if (m_token != TOKEN_COLON) {
                    Error("Expecting ':'");
                    goto recovery;
                }
                if (!Advance()) goto recovery; // absorb ':'
            } while (m_token == TOKEN_CASE);
            node->AddStatement(ParseStatement(false));
recovery:             
            if (on_error_) {
                // skips past the end of the next statement (at the beginnig of the next case!) 
                if (!SkipToNextStatement(recovery_level)) {
                    goto recovery2;
                }
            }
        }
        if (m_token == TOKEN_DEFAULT) {
            if (!Advance()) goto recovery2;
            if (m_token != TOKEN_COLON) {
                Error("Expecting ':'");
                goto recovery2;
            }
            if (!Advance()) goto recovery2;
            if (m_token == TOKEN_CURLY_CLOSE) {
                node->AddDefaultStatement(nullptr);
            } else {
                node->AddDefaultStatement(ParseStatement(false));
                if (on_error_) goto recovery2;
            }
            if (m_token != TOKEN_CURLY_CLOSE) {
                Error("Expecting '}', the else case must be the last of the switch !");
                goto recovery2;
            }
        }
        Advance();  // absorb '}'
        UpdateEndPosition(node);
    }
recovery2:    
    if (on_error_) {
        delete node;
        node = nullptr;
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
    {
        RecordPosition(node);
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_ROUND_OPEN) {
            Error("Expecting '('");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_NAME) {
            Error("Expecting the reference name");
            goto recovery2;
        }
        var = new VarDeclaration(m_lexer->CurrTokenString());
        RecordPosition(var);
        var->SetFlags(VF_IS_REFERENCE);
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_ASSIGN) {
            Error("Expecting '='");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;  // absorb '='
        IAstExpNode *exp = ParseExpression();
        if (on_error_) goto recovery2;
        node->Init(var, exp);
        var = nullptr;
        if (m_token != TOKEN_ROUND_CLOSE) {
            Error("Expecting ')'");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;
        if (m_token != TOKEN_CURLY_OPEN) {
            Error("Expecting '{'");
            goto recovery2;
        }
        if (!Advance()) goto recovery2;  // absorb '{'
        while (m_token != TOKEN_CURLY_CLOSE && m_token != TOKEN_DEFAULT) {
            int recovery_level = curly_indent_;
            IAstTypeNode *the_type = nullptr;
            {
                if (m_token != TOKEN_CASE) {
                    Error("Expecting 'case'");
                    goto recovery;
                }
                if (!Advance()) goto recovery;  // absorb 'case'
                the_type = ParseTypeSpecification();
                if (on_error_) goto recovery;
                if (m_token != TOKEN_COLON) {
                    Error("Expecting ':'");
                    goto recovery;
                }
                if (!Advance()) goto recovery; // absorb ':'
                node->AddCase(the_type, ParseStatement(false));
                the_type = nullptr;
            }
recovery:            
            if (on_error_) {
                if (the_type != nullptr) delete the_type;
                if (!SkipToNextStatement(recovery_level)) {
                    goto recovery2;
                }
            }
        }
        if (m_token == TOKEN_DEFAULT) {
            if (!Advance()) goto recovery2;
            if (m_token != TOKEN_COLON) {
                Error("Expecting ':'");
                goto recovery2;
            }
            if (!Advance()) goto recovery2;
            if (m_token == TOKEN_CURLY_CLOSE) {
                node->AddCase(nullptr, nullptr);
            } else {
                node->AddCase(nullptr, ParseStatement(false));
                if (on_error_) goto recovery2;
                if (m_token != TOKEN_CURLY_CLOSE) {
                    Error("Expecting '}', the else case must be the last of the switch !");
                    goto recovery2;
                }
            }
        }
        if (!Advance()) goto recovery2;  // absorb '}'
        UpdateEndPosition(node);
    }
recovery2:    
    if (on_error_) {
        delete node;
        if (var != nullptr) delete var;
        node = nullptr;
    }
    return(node);
}

void Parser::CheckSemicolon(void)
{
    if (m_token != TOKEN_SEMICOLON) {
        Error("Missing ';'");
        return;
    }
    Advance();
}

bool Parser::Advance(void)
{
    do {
        if (!m_lexer->Advance(&m_token)) {
            int row, col;
            string mess;
            m_lexer->GetError(&mess, &row, &col);
            SetError(mess.c_str(), row, col);
            m_lexer->ClearError();
            return(false);
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
    return(true);
}

void Parser::Error(const char *message)
{
    int line, col;

    line = m_lexer->CurrTokenLine();
    col = m_lexer->CurrTokenColumn() + 1;
    SetError(message, line, col);
}

void Parser::SetError(const char *message, int row, int column)
{
    // errors happening while trying to recover are ignored.
    if (on_error_) {
        return;
    }
    errors_->AddError(message, row, column);
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
    if (m_token == TOKEN_EOF) {
        // in this case didn't manage to solve the situation. Must keep returning from functions.
        on_error_ = true;
        return(false);
    }
    on_error_ = false;
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
        if (!m_lexer->Advance(&m_token)) {
            m_token = TOKEN_COMMENT;    // ignore errors: go on eating tokens 'til the first valid one.
            m_lexer->ClearError();
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