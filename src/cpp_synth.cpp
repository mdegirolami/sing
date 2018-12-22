#include <assert.h>
#include <string.h>
#include "cpp_synth.h"

namespace SingNames {

void CppSynth::Synthetize(FILE *fd, AstFile *root)
{
    int ii;

    file_ = fd;
    indent_ = 0;
    id_sequence_ = 0;
    for (ii = 0; ii < root->declarations_.size(); ++ii) {

        // check: verificare nome univoco, inserire in mappa.

        IAstNode *declaration = root->declarations_[ii];
        switch (declaration->GetType()) {
        case ANT_VAR:
            SynthVar((VarDeclaration*)declaration);
            break;
        case ANT_CONST:
            SynthConst((ConstDeclaration*)declaration);
            break;
        case ANT_TYPE:
            SynthType((TypeDeclaration*)declaration);
            break;
        case ANT_FUNC:
            SynthFunc((FuncDeclaration*)declaration);
            break;
        default:
            assert(false);
        }
    }
}

void CppSynth::SynthVar(VarDeclaration *declaration)
{
    string text, typedecl;

    if (declaration->volatile_flag_) {
        text = "static volatile ";
    }
    typedecl = declaration->name_;
    SynthTypeSpecification(&typedecl, declaration->type_spec_);
    text += typedecl;
    if (declaration->initer_ != NULL) {
        SynthIniter(&text, declaration->initer_);
    }
    Write(&text);
}

void CppSynth::SynthConst(ConstDeclaration *declaration)
{
    string text("static const "), typedecl;

    text = declaration->name_;
    SynthTypeSpecification(&text, declaration->type_spec_);
    SynthIniter(&text, declaration->initer_);
    Write(&text);
}

void CppSynth::SynthType(TypeDeclaration *declaration)
{
    string text("typedef "), typedecl;

    text = declaration->name_;
    SynthTypeSpecification(&text, declaration->type_spec_);
    Write(&text);
}

void CppSynth::SynthFunc(FuncDeclaration *declaration)
{
    string text, typedecl;

    if (declaration->is_class_member_) {
        text = declaration->classname_ + "::" + declaration->name_;
        SynthTypeSpecification(&text, declaration->function_type_);
    } else {
        text = declaration->name_;
        SynthTypeSpecification(&text, declaration->function_type_);
    }
    text += " {";
    Write(&text, false);
    SynthBlock(declaration->block_);
}

void CppSynth::SynthTypeSpecification(string *dst, IAstNode *type_spec)
{
    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
        {
            const char *name = GetBaseTypeName(((AstBaseType*)type_spec)->base_type_);
            dst->insert(0, name);
            dst->insert(strlen(name), " ");
        }
        break;
    case ANT_NAMED_TYPE:
        {

        // check: verificare esistenza.

            const char *name = ((AstNamedType*)type_spec)->name_.c_str();
            dst->insert(0, name);
            dst->insert(strlen(name), " ");
        }
        break;
    case ANT_QUALIFIED_TYPE:
        {
            string      fullname;

            // check: verificare esistenza.

            ((AstQualifiedType*)type_spec)->BuildTheFullName(&fullname);
            fullname += " ";
            dst->insert(0, fullname);
        }
        break;
    case ANT_ARRAY_TYPE:
        {
            AstArrayOrMatrixType *node = (AstArrayOrMatrixType*)type_spec;
            int     ii;
            char    intbuf[32];

            for (ii = 0; ii < node->dimensions_.size(); ++ii) {
                if (node->dimensions_[ii] >= 1) {
                    sprintf(intbuf, "[%d]", node->dimensions_[ii]);
                    *dst += intbuf;
                } else if (node->expressions_[ii] != NULL) {
                    string expression;

                    SynthExpression(&expression, node->expressions_[ii]);
                    *dst += '[';
                    *dst += expression;
                    *dst += '[';
                } else {
                    *dst += "[]";
                }
            }
            SynthTypeSpecification(dst, node->element_type_);
        }
        break;
    case ANT_MAP_TYPE:
        {
            AstMapType *node = (AstMapType*)type_spec;
            string fulldecl, the_type;

            fulldecl = "unordered_map<";
            SynthTypeSpecification(&the_type, node->key_type_);
            fulldecl += the_type;
            fulldecl += ", ";
            SynthTypeSpecification(&the_type, node->returned_type_);
            fulldecl += the_type;
            fulldecl += "> ";
            dst->insert(0, fulldecl);
        }
        break;
    case ANT_POINTER_TYPE:
        {
            AstPointerType *node = (AstPointerType*)type_spec;
            string fulldecl, the_type;

            fulldecl = "s_";
            if (node->isconst_) fulldecl += 'c';
            if (node->isweak_) fulldecl += 'w';
            fulldecl += "ptr<";
            SynthTypeSpecification(&the_type, node->pointed_type_);
            fulldecl += the_type;
            fulldecl += "> ";
            dst->insert(0, fulldecl);
        }
        break;
    case ANT_FUNC_TYPE:
        {
            AstFuncType *node = (AstFuncType*)type_spec;
            int ii;

            *dst += '(';
            if (node->arguments_.size() > 0) {
                int last_uninited;
                for (last_uninited = node->arguments_.size() - 1; last_uninited >= 0; --last_uninited) {
                    if (node->arguments_[last_uninited]->initer_ == NULL) break;
                }

                for (ii = 0; ii < node->arguments_.size(); ++ii) {
                    AstArgumentDecl *arg = node->arguments_[ii];
                    string the_type(arg->name_);

                    // check: if is a declaration, must specify in/out/io, else must determine scanning the lock !!

                    if (ii != 0) {
                        *dst += ", ";
                    }
                    SynthTypeSpecification(&the_type, arg->type_);
                    *dst += the_type;
                    if (ii > last_uninited) {
                        SynthIniter(&the_type, arg->initer_);
                    }
                }
                if (node->varargs_) {
                    *dst += ", ";
                }
            }
            if (node->varargs_) {
                *dst += "...";
            }
            *dst += ')';
            SynthTypeSpecification(dst, node->return_type_);
        }
        break;
    }
}

void CppSynth::SynthIniter(string *dst, IAstNode *initer)
{
    *dst += " = ";
    SynthIniterCore(dst, initer);
}

void CppSynth::SynthIniterCore(string *dst, IAstNode *initer)
{

    // check: consistenza con tipo o ritornare, appunto, il tipo.


    if (initer->GetType() == ANT_INITER) {
        AstIniter *ast_initer = (AstIniter*)initer;
        int ii;

        *dst += '{';
        for (ii = 0; ii < ast_initer->elements_.size(); ++ii) {
            if (ii != 0) *dst += ", ";
            SynthIniterCore(dst, ast_initer->elements_[ii]);
        }
        *dst += '}';
    } else {
        SynthExpression(dst, initer);
    }
}

void CppSynth::SynthBlock(AstBlock *block, bool write_closing_bracket)
{
    string  text;
    int     ii;

    ++indent_;
    for (ii = 0; ii < block->block_items_.size(); ++ii) {
        IAstNode *node = block->block_items_[ii];
        switch (node->GetType()) {
        case ANT_VAR:

            // check: duplication avoidance, insertion !!

            SynthVar((VarDeclaration*)node);
            break;
        case ANT_CONST:

            // check: duplication avoidance, insertion !!

            SynthConst((ConstDeclaration*)node);
            break;
        case ANT_ASSIGNMENT:
            SynthAssignment((AstAssignment*)node);
            break;
        case ANT_UPDATE:
            SynthUpdateStatement((AstUpdate*)node);
            break;
        case ANT_INCDEC:
            SynthIncDec((AstIncDec*)node);
            break;
        case ANT_WHILE:
            SynthWhile((AstWhile*)node);
            break;
        case ANT_IF:
            SynthIf((AstIf*)node);
            break;
        case ANT_FOR:
            SynthFor((AstFor*)node);
            break;
        case ANT_SIMPLE:
            SynthSimpleStatement((AstSimpleStatement*)node);
            break;
        case ANT_RETURN:
            SynthReturn((AstReturn*)node);
            break;
        }
    }
    --indent_;
    if (write_closing_bracket) {
        text = "}";
        Write(&text, false);
    }
}

void CppSynth::SynthAssignment(AstAssignment *node)
{
    int ii;
    string full, expression;

    // check: types compatibility (annotate the node).

    if (node->left_terms_.size() == 1) {
        SynthExpression(&full, node->left_terms_[0]);
        full += " = ";
        SynthExpression(&expression, node->right_terms_[0]);
        full += expression;
        Write(&full);
    } else {
        /*
        char buffer[30];

        for (ii = 0; ii < left_terms_.size(); ++ii) {
            CreateUniqueId(&id);
            full = id;
            full += "_"


        }
        */
    }
}

void CppSynth::SynthUpdateStatement(AstUpdate *node)
{
    string full, expression;

    // check: types compatibility (annotate the node).

    if (node->operation_ == TOKEN_UPD_POWER) {

    } else {
        SynthExpression(&full, node->left_term_);
        full += ' ';
        full += lexer_->GetTokenString(node->operation_);
        full += ' ';       
        SynthExpression(&expression, node->right_term_);
        full += expression;
        Write(&full);
    }
}

void CppSynth::SynthIncDec(AstIncDec *node)
{
    string text;

    // check: types compatibility (annotate the node).

    SynthExpression(&text, node->left_term_);
    text += lexer_->GetTokenString(node->operation_);
    Write(&text);
}

void CppSynth::SynthWhile(AstWhile *node)
{
    string text;

    // check: types compatibility (annotate the node).

    SynthExpression(&text, node->expression_);
    text.insert(0, "while (");
    text += ") {";
    Write(&text);
    SynthBlock(node->block_);
}

void CppSynth::SynthIf(AstIf *node)
{
    string  text;
    int     ii;

    // check: types compatibility (annotate the node).

    SynthExpression(&text, node->expressions_[0]);
    text.insert(0, "if (");
    text += ") {";
    Write(&text);
    SynthBlock(node->blocks_[0], false);
    for (ii = 1; ii < node->expressions_.size(); ++ii) {
        text = "";
        SynthExpression(&text, node->expressions_[ii]);
        text.insert(0, "} else if (");
        text += ") {";
        Write(&text);
        SynthBlock(node->blocks_[ii], false);
    }
    if (node->default_block_ != NULL) {
        text = "} else {";
        Write(&text);
        SynthBlock(node->default_block_, false);
    }
    text = "}";
    Write(&text, false);
}

void CppSynth::SynthFor(AstFor *node)
{
}

/*
string      index_name_;
string      iterator_name_;
IAstNode    *set_;
IAstNode    *low_;
IAstNode    *high_;
IAstNode    *step_;
AstBlock    *block_;
*/

void CppSynth::SynthSimpleStatement(AstSimpleStatement *node)
{
    // check: is in an inner block who is continuable/breakable ?

    string text = lexer_->GetTokenString(node->subtype_);
    Write(&text);
}

void CppSynth::SynthReturn(AstReturn *node)
{
    string text;

    // check: types compatibility (annotate the node).

    SynthExpression(&text, node->retvalue_);
    text.insert(0, "return (");
    text += ")";
    Write(&text);
}

void CppSynth::SynthExpression(string *dst, IAstNode *node, int father_priority)
{
    int priority = 0;

    // check: return type and costness.

    switch (node->GetType()) {
    case ANT_INDEXING:
        SynthIndices(dst, (AstIndexing*)node);
        priority = 2;
        break;
    case ANT_FUNCALL:
        SynthFunCall(dst, (AstFunCall*) node);
        priority = 2;
        break;
    case ANT_BINOP:
        priority = SynthBinop(dst, (AstBinop*)node);
        break;
    case ANT_UNOP:
        priority = SynthUnop(dst, (AstUnop*)node);
        break;
    case ANT_EXP_LEAF:
        SynthLeaf(dst, (AstExpressionLeaf*)node);
        break;
    }

    // a function-like operator: needs no protection 
    if (priority == 100) priority = 0;

    // protect the priority of this branch from adjacent operators
    if (father_priority < priority || priority > 3 && father_priority == priority ) {
        dst->insert(0, "(");
        *dst += ')';
    }
}

void CppSynth::SynthIndices(string *dst, AstIndexing *node)
{
    int     ii;
    string  expression;

    // check: types compatibility (annotate the node).

    SynthExpression(dst, node->left_term_, 2);
    for (ii = 0; ii < node->lower_values_.size(); ++ii) {
        expression = "";
        SynthExpression(dst, node->lower_values_[ii]);
        *dst += '[';
        *dst += expression;
        *dst += ']';
    }
}

void CppSynth::SynthFunCall(string *dst, AstFunCall *node)
{
    int     ii;
    string  expression;

    // check: types compatibility (annotate the node).

    SynthExpression(dst, node->left_term_, 2);
    *dst += '(';
    for (ii = 0; ii < node->arguments_.size(); ++ii) {
        expression = "";
        SynthExpression(dst, node->arguments_[ii]);
        if (ii != 0) {
            *dst += ' ,';
        }
        *dst += expression;
    }
    *dst += ')';
}

int CppSynth::SynthBinop(string *dst, AstBinop *node)
{
    int     ii, priority;
    string  expression;

    // check: types compatibility (annotate the node).

    priority = GetBinopCppPriority(node->subtype_);
    SynthExpression(dst, node->operand_left_, priority);
    switch (node->subtype_) {
    case TOKEN_POWER:
        break;
    case TOKEN_XOR:
        *dst += " ^ ";
        break;
    default:
        *dst += ' ';
        *dst += lexer_->GetTokenString(node->subtype_);
        *dst += ' ';
        break;
    }
    SynthExpression(&expression, node->operand_right_, priority);
    *dst += expression;
    return(priority);
    /*
    switch (node->subtype_) {
    case TOKEN_POWER:
        return(0);
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_MOD:
    case TOKEN_SHR:
    case TOKEN_SHL:
    case TOKEN_AND:
        return(1);
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_OR:
    case TOKEN_XOR:
        return(2);
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_GTE:
    case TOKEN_LTE:
    case TOKEN_DIFFERENT:
    case TOKEN_EQUAL:
        return(3);
    case TOKEN_LOGICAL_AND:
        return(4);
    case TOKEN_LOGICAL_OR:
    }
    */
}

int CppSynth::SynthUnop(string *dst, AstUnop *node)
{

    // check: types compatibility (annotate the node).

    int priority;

    if (node->subtype_ == TOKEN_SIZEOF || node->subtype_ == TOKEN_DIMOF) {
        priority = 100;
    } else {
        priority = 3;
    }
    SynthExpression(dst, node->operand_, priority);

    switch (node->subtype_) {
    case TOKEN_SIZEOF:
        dst->insert(0, "sizeof(");
        *dst += ')';
        break;
    case TOKEN_DIMOF:
        break;                      // TODO
    case TOKEN_MINUS:
    case TOKEN_PLUS:
    case TOKEN_AND:
    case TOKEN_NOT:
    case TOKEN_LOGICAL_NOT:
    case TOKEN_MPY:
        dst->insert(0, lexer_->GetTokenString(node->subtype_));
        break;
        /*
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
    */
    default:
        // all the types conversions
        char cast[20];

        sprintf(cast, "(%s)", GetBaseTypeName(node->subtype_));
        dst->insert(0, cast);
        break;
    }
    return(priority);
}

void CppSynth::SynthLeaf(string *dst, AstExpressionLeaf *node)
{
    switch (node->subtype_) {
    case TOKEN_NULL:
        *dst = "nullptr";
        break;
    case TOKEN_FALSE:
    case TOKEN_TRUE:
        *dst = lexer_->GetTokenString(node->subtype_);
        break;
    case TOKEN_LITERAL_STRING:
    case TOKEN_LITERAL_UINT:
    case TOKEN_LITERAL_FLOAT:
    case TOKEN_NAME:

        // check: var existence.

        *dst = node->value_;
        break;
    case TOKEN_LITERAL_IMG:

        // check required includes

        *dst = "complex<0,";
        *dst += lexer_->GetTokenString(node->subtype_);
        *dst += ">";
    }
}

void CppSynth::Write(string *text, bool add_semicolon = true)
{
    static const char *with = ";\r\n";
    static const char *wout = "\r\n";
    int ii;

    for (ii = indent_; ii > 0; --ii) {
        fwrite("    ", 4, 1, file_);
    }
    *text += add_semicolon ? with : wout;
    fwrite(&text[0], text->length(), 1, file_);
}

void CppSynth::AddNewLine(string *dst)
{
    int ii;

    *dst += "\r\n";
    for (ii = indent_; ii > 0; --ii) {
        *dst += "    ";
    }
}

const char *CppSynth::GetBaseTypeName(Token token)
{
    switch (token) {
    case TOKEN_INT8:
        return("int8_t");
    case TOKEN_INT16:
        return("int16_t");
    case TOKEN_INT32:
        return("int32_t");
    case TOKEN_INT64:
        return("int64_t");
    case TOKEN_UINT8:
        return("uint8_t");
    case TOKEN_UINT16:
        return("uint16_t");
    case TOKEN_UINT32:
        return("uint32_t");
    case TOKEN_UINT64:
        return("uint64_t");
    case TOKEN_FLOAT32:
        return("float");
    case TOKEN_FLOAT64:
        return("double");
    case TOKEN_COMPLEX64:
        return("complex<float>");
    case TOKEN_COMPLEX128:
        return("complex<double>");
    case TOKEN_STRING:
        return("string");
    case TOKEN_RUNE:
        return("int32_t");
    case TOKEN_BOOL:
        return("bool");
    case TOKEN_SIZE_T:
        return("size_t");
    case TOKEN_ERRORCODE:
        return("int32_t");
    case TOKEN_VOID:
        return("void");
    }
}

int  CppSynth::GetBinopCppPriority(Token token)
{
    switch (token) {
    case TOKEN_POWER:
        return(100);      // means "doesn't require parenthesys/doesn't take precedence over parents."
    case TOKEN_DOT:
        return(2);
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_MOD:
        return(5);
    case TOKEN_SHR:
    case TOKEN_SHL:
        return(7);
    case TOKEN_AND:
        return(11);
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return(6);
    case TOKEN_OR:
        return(13);
    case TOKEN_XOR:
        return(12);
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_GTE:
    case TOKEN_LTE:
        return(9);
    case TOKEN_DIFFERENT:
    case TOKEN_EQUAL:
        return(10);
    case TOKEN_LOGICAL_AND:
        return(14);
    case TOKEN_LOGICAL_OR:
        return(15);
    }
    return(100);
}

} // namespace