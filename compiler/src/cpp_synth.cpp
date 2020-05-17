#include <assert.h>
#include <string.h>
#include <vector>
#include "cpp_synth.h"
#include "FileName.h"
#include "helpers.h"
#include "builtin_functions.h"

namespace SingNames {

static const int KForcedPriority = 100;     // i.e. requires no protection and never overrides order. (uses brackets)
//static const int KLiteralPriority = 0;      // a single literal numeral, can be converted changing the suffix
static const int KLeafPriority = 1;         // a single item, not a literal numeral
static const int KCastPriority = 3;

bool IsFloatFormat(const char *num);

void CppSynth::Synthetize(FILE *cppfd, FILE *hfd, vector<Package*> *packages, int pkg_index, bool *empty_cpp)
{
    string      text;
    int         num_levels, num_items;

    cppfd_ = cppfd;
    hfd_ = hfd;
    packages_ = packages;
    root_ = (*packages)[pkg_index]->root_;
    indent_ = 0;
    split_level_ = 0xff;
    exp_level = 0;

    // HPP file
    /////////////////
    formatter_.Reset();
    file_ = hfd;
    text = "#pragma once";
    Write(&text, false);
    EmptyLine();

    // headers
    text = "#include <sing.h>";
    Write(&text, false);
    WriteHeaders(DependencyUsage::PUBLIC);
    EmptyLine();

    // open the namespace
    num_levels = WriteNamespaceOpening();

    // all public declarations by cathegory
    WriteClassForwardDeclarations(true);
    WriteTypeDefinitions(true);
    WritePrototypes(true);
    WriteExternalDeclarations();

    WriteNamespaceClosing(num_levels);

    // CPP file
    /////////////////
    formatter_.Reset();
    file_ = cppfd;
    FileName::SplitFullName(nullptr, &text, nullptr, &(*packages)[pkg_index]->fullpath_);
    text.insert(0, "#include \"");
    text += ".h\"";
    Write(&text, false);
    num_items = WriteHeaders(DependencyUsage::PRIVATE);
    EmptyLine();

    // open the namespace
    num_levels = WriteNamespaceOpening();

    // all public declarations by cathegory
    WriteClassForwardDeclarations(false);
    num_items += WriteTypeDefinitions(false);
    WritePrototypes(false);
    num_items += WriteVariablesDefinitions();
    num_items += WriteClassIdsDefinitions();
    num_items += WriteConstructors();
    num_items += WriteFunctions();

    WriteNamespaceClosing(num_levels);

    *empty_cpp = num_items == 0;
}

void CppSynth::SynthVar(VarDeclaration *declaration)
{
    string  text, typedecl, initer;

    if (!declaration->HasOneOfFlags(VF_ISLOCAL) && 
        (!declaration->IsPublic() || declaration->HasOneOfFlags(VF_INVOLVED_IN_TYPE_DEFINITION))
        ) {
        text = "static ";
    }
    if (declaration->initer_ != nullptr) {
        SynthIniterCore(&initer, declaration->weak_type_spec_, declaration->initer_);
    } else if (declaration->HasOneOfFlags(VF_ISLOCAL)) {
        SynthZeroIniter(&initer, declaration->weak_type_spec_);
    }
    if (declaration->HasOneOfFlags(VF_ISPOINTED)) {
        bool init_on_second_row = declaration->initer_ != nullptr && declaration->initer_->GetType() == ANT_INITER;

        text += "sing::";
        if (declaration->HasOneOfFlags(VF_READONLY)) {
            text += "c";
        }
        text += "ptr<";
        SynthTypeSpecification(&typedecl, declaration->weak_type_spec_);
        if (declaration->HasOneOfFlags(VF_ISVOLATILE)) {
            typedecl.insert(0, "volatile");
        }
        text += typedecl;
        text += "> ";
        text += declaration->name_;
        text += "(new sing::wrapper<";
        text += typedecl;
        if (initer.length() > 0 && !init_on_second_row) {
            text += ">(";
            text += initer;
            text += "))";
        } else {
            text += ">)";
        }
        Write(&text);
        if (initer.length() > 0 && init_on_second_row) {
            text = "*";
            text += declaration->name_;
            text += " = ";
            text += initer;
            Write(&text);
        }
    } else {
        if (declaration->HasOneOfFlags(VF_ISVOLATILE)) {
            text += "volatile ";
        } else if (declaration->HasOneOfFlags(VF_READONLY)) {
            text += "const ";
        }
        typedecl = declaration->name_;
        SynthTypeSpecification(&typedecl, declaration->weak_type_spec_);
        text += typedecl;
        if (initer.length() > 0) {
            text += " = ";
            text += initer;
        }
        Write(&text);
    }
}

void CppSynth::SynthType(TypeDeclaration *declaration)
{
    switch (declaration->type_spec_->GetType()) {
        case ANT_CLASS_TYPE:
            SynthClassDeclaration(declaration->name_.c_str(), (AstClassType*)declaration->type_spec_);
            break;
        case ANT_INTERFACE_TYPE:
            SynthInterfaceDeclaration(declaration->name_.c_str(), (AstInterfaceType*)declaration->type_spec_);
            break;
        case ANT_ENUM_TYPE:
            SynthEnumDeclaration(declaration->name_.c_str(), (AstEnumType*)declaration->type_spec_);
            break;
        default:
        {
            string text(declaration->name_);

            SynthTypeSpecification(&text, declaration->type_spec_);
            text.insert(0, "typedef ");
            Write(&text);
            break;
        }
    }
}

void CppSynth::SynthFunc(FuncDeclaration *declaration)
{
    string text, typedecl;

    EmptyLine();
    if (declaration->is_class_member_) {
        AstClassType *ctype = GetLocalClassTypeDeclaration(declaration->classname_.c_str());
        if (ctype != nullptr && ctype->has_constructor && !ctype->constructor_written) {
            ctype->SetConstructorDone();
            SynthConstructor(&declaration->classname_, ctype);
        }
        EmptyLine();
        if (declaration->name_ == "finalize") {
            text = declaration->classname_ + "::~" + declaration->classname_ + "()";
        } else {
            text = declaration->classname_ + "::" + declaration->name_;
            SynthFuncTypeSpecification(&text, declaration->function_type_, false);
            if (!declaration->is_muting_) {
                text += " const";
            }
        }
    } else {
        text = declaration->name_;
        SynthFuncTypeSpecification(&text, declaration->function_type_, false);
        if (!declaration->IsPublic()) {
            text.insert(0, "static ");
        }
    }
    SynthFunOpenBrace(text);
    return_type_ = GetBaseType(declaration->function_type_->return_type_);
    SynthBlock(declaration->block_);
}

void CppSynth::SynthFunOpenBrace(string &text)
{
    if (!newline_before_function_bracket_) {
        text += " {";
    }
    Write(&text, false);
    if (newline_before_function_bracket_) {
        text = "{";
        Write(&text, false);
    }
}

void CppSynth::SynthConstructor(string *classname, AstClassType *ctype)
{
    string initer, text;

    EmptyLine();
    text = *classname + "::" + *classname + "()";
    SynthFunOpenBrace(text);
    ++indent_;

    for (int ii = 0; ii < ctype->member_vars_.size(); ++ii) {
        VarDeclaration *vdecl = ctype->member_vars_[ii];
        if (vdecl->initer_ != nullptr) {
            SynthIniterCore(&initer, vdecl->weak_type_spec_, vdecl->initer_);
        } else {
            SynthZeroIniter(&initer, vdecl->weak_type_spec_);
        }
        if (initer.length() > 0) {
            text = "";
            AppendMemberName(&text, vdecl);
            text += " = ";
            text += initer;
            Write(&text);
        }
    }

    --indent_;
    text = "}";
    Write(&text, false);
}

void CppSynth::SynthTypeSpecification(string *dst, IAstTypeNode *type_spec, bool root_of_fun_parm)
{
    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
        {
            const char *name = GetBaseTypeName(((AstBaseType*)type_spec)->base_type_);
            PrependWithSeparator(dst, name);
        }
        break;
    case ANT_NAMED_TYPE:
        {
            AstNamedType *node = (AstNamedType*)type_spec;
            if (node->next_component != nullptr) {
                string full;

                GetFullExternName(&full, node->pkg_index_, node->next_component->name_.c_str());
                PrependWithSeparator(dst, full.c_str());
            } else {
                PrependWithSeparator(dst, node->name_.c_str());
            }
        }
        break;
    case ANT_ARRAY_TYPE:
        SynthArrayTypeSpecification(dst, (AstArrayType*)type_spec, root_of_fun_parm);
        break;
    case ANT_MAP_TYPE:
        {
            AstMapType *node = (AstMapType*)type_spec;
            string fulldecl, the_type;

            fulldecl = "std::unordered_map<";
            SynthTypeSpecification(&the_type, node->key_type_);
            fulldecl += the_type;
            fulldecl += ", ";
            SynthTypeSpecification(&the_type, node->returned_type_);
            fulldecl += the_type;
            fulldecl += ">";
            PrependWithSeparator(dst, fulldecl.c_str());
        }
        break;
    case ANT_POINTER_TYPE:
        {
            AstPointerType *node = (AstPointerType*)type_spec;
            string fulldecl, the_type;

            fulldecl = "sing::";
            if (node->pointed_type_->GetType() == ANT_NAMED_TYPE) {
                AstNamedType *namenode = (AstNamedType*)node->pointed_type_;
                if (namenode->wp_decl_->GetType() == ANT_TYPE) {
                    TypeDeclaration *tdecl = (TypeDeclaration*)namenode->wp_decl_;
                    if (tdecl->type_spec_->GetType() == ANT_INTERFACE_TYPE) {
                        fulldecl += 'i';
                    }
                }
            }
            if (node->isconst_) fulldecl += 'c';
            if (node->isweak_) fulldecl += 'w';
            fulldecl += "ptr<";
            SynthTypeSpecification(&the_type, node->pointed_type_);
            fulldecl += the_type;
            fulldecl += ">";
            PrependWithSeparator(dst, fulldecl.c_str());
        }
        break;
    case ANT_FUNC_TYPE:
        dst->insert(0, "(*");
        *dst += ")";
        SynthFuncTypeSpecification(dst, (AstFuncType*)type_spec, false);
        break;
    }
}

void CppSynth::SynthFuncTypeSpecification(string *dst, AstFuncType *type_spec, bool prototype)
{
    int ii;

    --split_level_;
    *dst += "(";
    AddSplitMarker(dst);
    if (type_spec->arguments_.size() > 0) {
        string  the_type;
        int     last_uninited;

        for (last_uninited = type_spec->arguments_.size() - 1; last_uninited >= 0; --last_uninited) {
            if (type_spec->arguments_[last_uninited]->initer_ == NULL) break;
        }

        for (ii = 0; ii < (int)type_spec->arguments_.size(); ++ii) {

            // collect info
            VarDeclaration *arg = type_spec->arguments_[ii];
            ParmPassingMethod mode = GetParameterPassingMethod(arg->weak_type_spec_, arg->HasOneOfFlags(VF_READONLY));

            // sinth the parm
            if (mode == PPM_POINTER) {
                the_type = "*";
                the_type += arg->name_;
            } else if (mode == PPM_CONSTREF || mode == PPM_REF) {
                the_type = "&";
                the_type += arg->name_;
            } else {    // PPM_VALUE
                the_type = arg->name_;
            }
            SynthTypeSpecification(&the_type, arg->weak_type_spec_, true);

            // add to dst
            if (ii != 0) {
                *dst += ", ";
                AddSplitMarker(dst);
            }
            if (arg->HasOneOfFlags(VF_READONLY)/* && mode == PPM_POINTER*/) {
                *dst += "const ";
            }
            if (ii > last_uninited && prototype) {
                SynthIniter(&the_type, arg->weak_type_spec_, arg->initer_);
            }
            *dst += the_type;
        }
        if (type_spec->varargs_) {
            *dst += ", ";
        }
    }
    if (type_spec->varargs_) {
        *dst += "...";
    }
    *dst += ')';
    SynthTypeSpecification(dst, type_spec->return_type_);
    ++split_level_;
}

void CppSynth::SynthArrayTypeSpecification(string *dst, AstArrayType *type_spec, bool root_of_fun_parm)
{
    char    intbuf[32];
    int     ndims = 1;
    string  the_type, decl;

    while (type_spec->is_regular) {
        type_spec = (AstArrayType*)type_spec->element_type_;
        ++ndims;
    }
    bool is_pod = IsPOD(type_spec->element_type_);
    if (ndims == 1) {
        decl = "sing::";
        if (root_of_fun_parm) {
            decl += "vect<";
        } else {
            decl += type_spec->is_dynamic_ ? 'd' : 's';
            decl += is_pod ? "pvect<" : "vect<";
        }
        SynthTypeSpecification(&the_type, type_spec->element_type_);
        decl += the_type;
        if (!type_spec->is_dynamic_ && !root_of_fun_parm) {
            if (type_spec->expression_ != nullptr) {
                string exp;

                decl += ", ";
                int priority = SynthExpression(&exp, type_spec->expression_);
                Protect(&exp, priority, GetBinopCppPriority(TOKEN_SHR));    // because SHR gets confused with the end of the template parameters list '>'
                decl += exp;
            } else {
                // length determined based on the initializer
                char buffer[32];
                sprintf(buffer, ", %llu", (uint64_t)type_spec->dimension_);
                decl += buffer;
            }
        }
        decl += ">";
    } else {
        sprintf(intbuf, "%d", ndims);
        decl = "sing::array<";
        SynthTypeSpecification(&the_type, type_spec->element_type_);
        decl += the_type;
        decl += ", ";
        decl += intbuf;
        decl += ">";
    }
    PrependWithSeparator(dst, decl.c_str());
}

void CppSynth::SynthClassDeclaration(const char *name, AstClassType *type_spec)
{
    bool has_base = type_spec->member_interfaces_.size() > 0;
    SynthClassHeader(name, &type_spec->member_interfaces_, false);

    // collect some info
    bool supports_typeswitch = type_spec->member_interfaces_.size() > 0;
    bool has_private = false;
    bool needs_constructor = false;
    for (int ii = 0; ii < type_spec->member_vars_.size(); ++ii) {
        VarDeclaration *vdecl = type_spec->member_vars_[ii];
        if (!vdecl->IsPublic()) {
            has_private = true;
        }
        if (vdecl->initer_ != nullptr || vdecl->weak_type_spec_->NeedsZeroIniter()) {
            needs_constructor = true;
        }
    }
    for (int ii = 0; ii < type_spec->member_functions_.size() && !has_private; ++ii) {
        if (!type_spec->member_functions_[ii]->IsPublic()) {
            has_private = true;
        }
    }

    // for later use !!
    if (needs_constructor) {
        type_spec->SetNeedsConstructor();
    }

    string text = "public:";
    Write(&text, false);

    // the functions
    ++indent_;

    // constructor
    if (needs_constructor) {
        text = name;
        text += "()";
        Write(&text);
    }

    // destructor
    if (type_spec->has_destructor) {
        text = name;
        if (has_base) {
            text.insert(0, "virtual ~");
            text += "()";
        } else {
            text.insert(0, "~");
            text += "()";
        }
        Write(&text);

        // if has a destructor, copying is unsafe !!
        /*
        text = name;
        text += "(const ";
        text += name;
        text += " &) = delete";
        Write(&text);

        text = name;
        text += " &operator=(const ";
        text += name;
        text += " &) = delete";
        Write(&text);
        */
    }

    // get__id (if inherits from an interface)
    if (supports_typeswitch) {
        if (use_override_) {
            text = "virtual void *get__id() const override { return(&id__); }";
        } else {
            text = "virtual void *get__id() const { return(&id__); }";
        }
        Write(&text);
    }

    // user defined
    int num_functions = SynthClassMemberFunctions(&type_spec->member_functions_, &type_spec->fn_implementors_, 
                                                    type_spec->first_hinherited_member_, true, false);
    if (num_functions > 0) {
        EmptyLine();
    }

    // the variables
    if (supports_typeswitch) {
        text = "static char id__";
        Write(&text);
    }
    SynthClassMemberVariables(&type_spec->member_vars_, true);

    --indent_;

    if (has_private) {
        EmptyLine();
        string text = "private:";
        Write(&text, false);

        ++indent_;
        num_functions = SynthClassMemberFunctions(&type_spec->member_functions_, &type_spec->fn_implementors_, 
                                                    type_spec->first_hinherited_member_, false, false);
        if (num_functions > 0) {
            EmptyLine();
        }
        SynthClassMemberVariables(&type_spec->member_vars_, false);
        --indent_;
    }

    // close the declaration
    text = "}";
    Write(&text);
}

void CppSynth::SynthClassHeader(const char *name, vector<AstNamedType*> *bases, bool is_interface)
{
    string text = "class ";
    text += name;
    if (!is_interface && use_final_) {
        text += " final";
    }
    int num_bases = bases->size();
    if (num_bases > 0) {
        string basename;
        text += " :";
        for (int ii = 0; ii < num_bases; ++ii) {
            SynthTypeSpecification(&basename, (*bases)[ii]);
            text += " public ";
            text += basename;
            if (ii < num_bases - 1) {
                text += ",";
            }
        }
    }
    text += " {";
    Write(&text, false);
}

int CppSynth::SynthClassMemberFunctions(vector<FuncDeclaration*> *declarations, vector<string> *implementors,
                                         int first_hinerited, bool public_members, bool is_interface)
{
    string text;
    int num_functions = 0;
    int top = declarations->size();

    // on interfaces there is no need to declare inherited functions
    if (is_interface) {
        top = first_hinerited;  
    }
    for (int ii = 0; ii < top; ++ii) {
        FuncDeclaration *func = (*declarations)[ii];
        if (func->IsPublic() != public_members) continue;
        if (func->name_ == "finalize") continue;    // declared elsewhere
        AstFuncType *ftype = func->function_type_;
        assert(ftype != nullptr);
        text = func->name_;
        SynthFuncTypeSpecification(&text, ftype, true);
        if (is_interface || ii >= first_hinerited) {
            text.insert(0, "virtual ");
        }
        if (!func->is_muting_) {
            text += " const";
        }
        if (!is_interface && ii >= first_hinerited && use_override_) {
            text += " override";
        }
        if (is_interface) {
            text += " = 0";
        } else if ((*implementors)[ii] != "") {
            SynthFunOpenBrace(text);
            ++indent_;
            bool voidfun = ftype->ReturnsVoid();
            if (!voidfun) {
                text = "return(";
            } else {
                text = "";
            }
            text += member_prefix_ + (*implementors)[ii] + member_suffix_ + "." + func->name_ + "(";
            --split_level_;
            for (int ii = 0; ii < ftype->arguments_.size(); ++ii) {
                text += ftype->arguments_[ii]->name_;
                if (ii < ftype->arguments_.size() - 1) {
                    text += ", ";
                    AddSplitMarker(&text);
                }
            }
            ++split_level_;
            text += ")";
            if (!voidfun) {
                text += ")";
            }
            Write(&text);

            --indent_;
            text = "}";
        }
        Write(&text);
        ++num_functions;
    } 
    return(num_functions);
}

void CppSynth::SynthClassMemberVariables(vector<VarDeclaration*> *d_vector, bool public_members)
{
    string text;
    int top = d_vector->size();

    for (int ii = 0; ii < top; ++ii) {
        VarDeclaration *declaration = (*d_vector)[ii];
        if (declaration->IsPublic() != public_members) continue;
        text = "";
        AppendMemberName(&text, declaration);
        SynthTypeSpecification(&text, declaration->weak_type_spec_);
        Write(&text);
    }
}

void CppSynth::SynthInterfaceDeclaration(const char *name, AstInterfaceType *type_spec)
{
    SynthClassHeader(name, &type_spec->ancestors_, true);

    string text = "public:";
    Write(&text, false);

    // the functions
    ++indent_;

    // virtual destructor and typeswitch support (if not inherited)
    if (type_spec->ancestors_.size() == 0) {
        text = "virtual ~";
        text += name;
        text += "() {}";
        Write(&text, false);      

        text = "virtual void *get__id() const = 0";
        Write(&text);      
    }

    SynthClassMemberFunctions(&type_spec->members_, nullptr, type_spec->first_hinherited_member_, true, true);
    --indent_;

    // close the declaration
    text = "}";
    Write(&text);
}

void CppSynth::SynthEnumDeclaration(const char *name, AstEnumType *type_spec)
{
    --split_level_;
    string text = "enum class ";
    text += name;
    text += " {";
    AddSplitMarker(&text);
    for (int ii = 0; ii < type_spec->items_.size(); ++ii) {
        text += type_spec->items_[ii];
        if (type_spec->initers_[ii] != nullptr) {
            string exp;
            SynthExpression(&exp, type_spec->initers_[ii]);
            text += " = ";
            text += exp;
        }
        if (ii < type_spec->items_.size() - 1) {
            text += ", ";
            AddSplitMarker(&text);
        }
    }
    text += "}";
    Write(&text);
    ++split_level_;
}

void CppSynth::SynthIniter(string *dst, IAstTypeNode *type_spec, IAstNode *initer)
{
    *dst += " = ";
    SynthIniterCore(dst, type_spec, initer);
}

void CppSynth::SynthIniterCore(string *dst, IAstTypeNode *type_spec, IAstNode *initer)
{
    if (initer->GetType() == ANT_INITER) {
        AstIniter *ast_initer = (AstIniter*)initer;
        int ii;
        int oldrow = 0;

        //--split_level_;
        *dst += '{';
        if (ast_initer->elements_.size() > 0) {
            if (initer->GetPositionRecord()->start_row < ast_initer->elements_[0]->GetPositionRecord()->start_row) {
                *dst += 0xff;
            }
            //AddSplitMarker(dst);
            for (ii = 0; ii < (int)ast_initer->elements_.size(); ++ii) {
                IAstNode *element = ast_initer->elements_[ii];
                if (ii != 0) {
                    *dst += ", ";
                    if (element->GetPositionRecord()->start_row > oldrow) {
                        *dst += 0xff;
                    } else {
                        //AddSplitMarker(dst);
                    }
                }
                oldrow = element->GetPositionRecord()->last_row;
                SynthIniterCore(dst, type_spec, element);
            }
            if (initer->GetPositionRecord()->last_row > ast_initer->elements_[ast_initer->elements_.size()-1]->GetPositionRecord()->last_row) {
                *dst += 0xff;
            }
            *dst += '}';
        }
        //++split_level_;
    } else {
        string exp;

        SynthFullExpression(GetBaseType(type_spec), &exp, (IAstExpNode*)initer);
        *dst += exp;
    }
}

void CppSynth::SynthZeroIniter(string *dst, IAstTypeNode *type_spec)
{
    *dst = "";

    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
        switch (((AstBaseType*)type_spec)->base_type_) {
        default:
            *dst = "0";
            break;
        case TOKEN_COMPLEX64:
        case TOKEN_COMPLEX128:
        case TOKEN_STRING:
            break;
        case TOKEN_BOOL:
            *dst = "false";
            break;
        }
        break;
    case ANT_NAMED_TYPE:
        SynthZeroIniter(dst, ((AstNamedType*)type_spec)->wp_decl_->type_spec_);
        break;
    case ANT_FUNC_TYPE:
        *dst = "nullptr";
        break;
    case ANT_ENUM_TYPE:
        *dst = ((AstEnumType*)type_spec)->items_[0];
        break;
    default:
        break;  
    }
}

void CppSynth::SynthBlock(AstBlock *block, bool write_closing_bracket)
{
    int         ii;
    AstNodeType oldtype = ANT_BLOCK;    // init so that SynthStatementOrAutoVar doesn't place an empty line.

    ++indent_;
    for (ii = 0; ii < (int)block->block_items_.size(); ++ii) {
        SynthStatementOrAutoVar(block->block_items_[ii], &oldtype);
    }
    --indent_;
    if (write_closing_bracket) {
        string text = "}";
        Write(&text, false);
    }
}

void CppSynth::SynthStatementOrAutoVar(IAstNode *node, AstNodeType *oldtype)
{
    string      text;
    AstNodeType type;

    type = node->GetType();
    formatter_.SetNodePos(node->GetPositionRecord(), type != ANT_VAR && type != ANT_BLOCK);

    // place an empty line before the first non-var statement following one or more var declarations 
    if (oldtype != nullptr) {
        if (type != ANT_VAR && *oldtype == ANT_VAR) {
            EmptyLine();
        }
        *oldtype = type;
    }

    switch (type) {
    case ANT_VAR:
        SynthVar((VarDeclaration*)node);
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
    case ANT_FUNCALL:
        text = "";
        SynthFunCall(&text, (AstFunCall*)node);
        Write(&text, true);
        break;
    case ANT_SWITCH:
        SynthSwitch((AstSwitch*)node);
        break;
    case ANT_TYPESWITCH:
        SynthTypeSwitch((AstTypeSwitch*)node);
        break;
    case ANT_BLOCK:
        text = "{";
        Write(&text, false);
        SynthBlock((AstBlock*)node);
        break;
    }
}

void CppSynth::SynthUpdateStatement(AstUpdate *node)
{
    string full;

    // if (node->operation_ == TOKEN_UPD_POWER) {
    //     SynthPowerUpdateOperator(&full, node);
    //     Write(&full);
    // } else {
        string expression;

        SynthExpression(&full, node->left_term_);
        full += ' ';
        full += Lexer::GetTokenString(node->operation_);
        full += ' ';       
        Token base_type = node->left_term_->GetAttr()->GetAutoBaseType();
        SynthFullExpression(base_type, &expression, node->right_term_);
        full += expression;
        Write(&full);
    // }
}

void CppSynth::SynthPowerUpdateOperator(string *dst, AstUpdate *node)
{
    string          right, left;
    bool            integer_power = false;

    int left_priority = SynthExpression(&left, node->left_term_);
    int right_priority = SynthExpression(&right, node->right_term_);

    const ExpressionAttributes *left_attr = node->left_term_->GetAttr();
    const ExpressionAttributes *right_attr = node->right_term_->GetAttr();

    const NumericValue *left_value = left_attr->GetValue();
    const NumericValue *right_value = right_attr->GetValue();

    Token target = left_attr->GetAutoBaseType();
    Token right_type = right_attr->GetAutoBaseType();

    *dst = left;

    // pow2 ?
    if (right_attr->HasKnownValue() && !right_value->IsComplex() && right_value->GetDouble() == 2) {
        *dst += " = sing::pow2(";
        *dst += left;
        *dst += ")";
    } else {
        if (left_attr->IsInteger()) {
            if (!right_attr->IsInteger()) {
                assert(false);  // shouldn't happen
                right_priority = CastIfNeededTo(target, right_type, &right, right_priority, true);
            }
            *dst += " = sing::pow(";
        } else {
            if (ExpressionAttributes::BinopRequiresNumericConversion(left_attr, right_attr, TOKEN_POWER)) {
                assert(false);  // shouldn't happen
                right_priority = CastIfNeededTo(target, right_type, &right, right_priority, true);
            }
            *dst += " = std::pow(";
        }
        *dst += left;
        *dst += ", ";
        *dst += right;
        *dst += ")";
    }
}

void CppSynth::SynthIncDec(AstIncDec *node)
{
    int priority;
    string text;

    priority = SynthExpression(&text, node->left_term_);
    Protect(&text, priority, GetUnopCppPriority(node->operation_));
    text.insert(0, Lexer::GetTokenString(node->operation_));
    Write(&text);
}

void CppSynth::SynthWhile(AstWhile *node)
{
    string text;

    SynthExpression(&text, node->expression_);
    text.insert(0, "while (");
    text += ") {";
    Write(&text, false);
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
    Write(&text, false);
    SynthBlock(node->blocks_[0], false);
    for (ii = 1; ii < (int)node->expressions_.size(); ++ii) {
        text = "";
        SynthExpression(&text, node->expressions_[ii]);
        text.insert(0, "} else if (");
        text += ") {";
        Write(&text, false);
        SynthBlock(node->blocks_[ii], false);
    }
    if (node->default_block_ != NULL) {
        text = "} else {";
        Write(&text, false);
        SynthBlock(node->default_block_, false);
    }
    text = "}";
    Write(&text, false);
}

void CppSynth::SynthSwitch(AstSwitch *node)
{
    string text;
    --split_level_;
    if (node->c_switch_compatible) {
        SynthExpression(&text, node->switch_value_);
        text.insert(0, "switch (");
        text += ") {";
        Write(&text, false);
        int cases = 0;
        for (int ii = 0; ii < (int)node->statements_.size(); ++ii) {
            int top_case = node->statement_top_case_[ii];
            if (top_case == cases) {
                text = "default:";
                Write(&text, false);
            } else {
                while (cases < top_case) {
                    IAstExpNode *clause = node->case_values_[cases];
                    if (clause != nullptr) {
                        SynthExpression(&text, clause);
                        text.insert(0, "case ");
                        text += ": ";
                       Write(&text, false);
                    }
                    ++cases;
                }
            }
            IAstNode *statement = node->statements_[ii];
            ++indent_;
            if (statement != nullptr) {
                SynthStatementOrAutoVar(statement, nullptr);
            }
            text = "break";
            Write(&text);            
            --indent_;
        }
    } else {
        int cases = 0;
        for (int ii = 0; ii < (int)node->statements_.size(); ++ii) {
            int top_case = node->statement_top_case_[ii];
            IAstNode *statement = node->statements_[ii];
            if (top_case == cases) {
                if (statement != nullptr) {
                    assert(ii != 0);
                    text = "} else {";
                    Write(&text, false);
                }
            } else {
                bool first = true;
                while (cases < top_case) {
                    if (first) {
                        SynthRelationalOperator3(&text, TOKEN_EQUAL, node->switch_value_, node->case_values_[cases]);
                    } else {
                        string single_clause;
                        SynthRelationalOperator3(&single_clause, TOKEN_EQUAL, node->switch_value_, node->case_values_[cases]);
                        text += " || ";
                        text += single_clause;
                    }
                    first = false;
                    ++cases;
                }
                if (ii == 0) {
                    text.insert(0, "if (");
                } else {
                    text.insert(0, "} else if (");
                }
                text += ") {";                    
                Write(&text, false);
            }
            if (statement != nullptr) {

                // this is all about avoiding double {}
                if (statement->GetType() == ANT_BLOCK) {
                    SynthBlock((AstBlock*)statement, false);
                } else {
                    ++indent_;
                    SynthStatementOrAutoVar(statement, nullptr);
                    --indent_;
                }
            }
        }
    }
    text = "}";
    ++split_level_;
    Write(&text, false);            
}

void CppSynth::SynthTypeSwitch(AstTypeSwitch *node)
{
    string text, switch_exp, tocompare;

    int exppri = SynthExpression(&switch_exp, node->expression_);
    tocompare = switch_exp;
    if (node->on_interface_ptr_) {
        Protect(&tocompare, exppri, GetUnopCppPriority(TOKEN_MPY));
        tocompare.insert(0, "(*");
        tocompare += ").get__id() == &";
    } else {
        Protect(&tocompare, exppri, GetBinopCppPriority(TOKEN_DOT));
        tocompare += ".get__id() == &";
    }
    for (int ii = 0; ii < node->case_types_.size(); ++ii) {
        IAstTypeNode *clause = node->case_types_[ii];
        IAstNode *statement = node->case_statements_[ii];
        bool needs_reference = node->uses_reference_[ii];
        string clause_typename;

        if (clause == nullptr) {
            if (statement != nullptr) {
                assert(ii != 0);
                text = "} else {";
                Write(&text, false);
            }
        } else {
            if (ii == 0) {
                text = "if (";
            } else {
                text = "} else if (";
            }
            text += tocompare;
            if (node->on_interface_ptr_) {
                IAstTypeNode *solved = SolveTypedefs(clause);
                if (solved->GetType() == ANT_POINTER_TYPE) {
                    SynthTypeSpecification(&clause_typename, ((AstPointerType*)solved)->pointed_type_);
                }
            } else {
                SynthTypeSpecification(&clause_typename, clause);
            }
            text += clause_typename;
            text += "::id__) {";
            Write(&text, false);
        }
        if (statement != nullptr) {

            ++indent_;

            // init the reference
            if (needs_reference) {
                if (node->on_interface_ptr_) {
                    // es: sing::ptr<Derived> localname = (sing::wrapper<Derived>*)p04.get_wrapper();
                    text = node->reference_->name_;
                    SynthTypeSpecification(&text, clause);
                    text += " = (sing::wrapper<";
                    text += clause_typename;
                    text += ">*)";
                    text += switch_exp;
                    text += ".get_wrapper()";
                } else {
                    // es: Derived *localname = (Derived *)&inparm;
                    text = clause_typename;
                    text += " *";
                    text += node->reference_->name_;
                    text += " = (";
                    text += clause_typename;
                    text += " *)&";
                    text += switch_exp;
                }
                Write(&text);
            }
                       
            // this is all about avoiding double {}
            if (statement->GetType() == ANT_BLOCK) {
                --indent_;
                SynthBlock((AstBlock*)statement, false);
                ++indent_;
            } else {
                SynthStatementOrAutoVar(statement, nullptr);
            }
            --indent_;
        }
    }    
    text = "}";
    Write(&text, false);            
}

void CppSynth::SynthFor(AstFor *node)
{
    string  text;

    // declare or init the index
    // (can't do in the init clause of the for because it is a declaration, not a statement !!
    if (node->index_ != NULL) {
        if (!node->index_referenced_) {
            text = "uint64_t ";
            text += node->index_->name_;
            text += " = 0";
            Write(&text);
        } else {
            text = node->index_->name_;
            text += " = 0";
            Write(&text);
        }
    }
    /* NO ! the iterator is in the block scope !!
    if (!node->iterator_referenced_) {
        if (node->iterator_->HasOneOfFlags(VF_IS_REFERENCE)) {
            text = "*";
        } else {
            text = "";
        }
        text += node->iterator_->name_;
        SynthTypeSpecification(&text, node->iterator_->weak_type_spec_);
        Write(&text);
    }
    */
    if (node->set_ != NULL) {
        SynthForEachOnDyna(node);
    } else {
        SynthForIntRange(node);
    }
}

void CppSynth::SynthForEachOnDyna(AstFor *node)
{
    string  expression, text;
    int     priority;

    --split_level_;
    text = "for(";
    AddSplitMarker(&text);

    // iterator declaration
    expression = "*";
    expression += node->iterator_->name_;
    SynthTypeSpecification(&expression, node->iterator_->weak_type_spec_);
    text += expression;

    // iterator initialization
    text += " = ";
    priority = SynthExpression(&expression, node->set_);
    Protect(&expression, priority, GetBinopCppPriority(TOKEN_DOT));
    text += expression;
    text += ".begin(); ";
    AddSplitMarker(&text);

    // end of loop clause and iterator increment
    text += node->iterator_->name_;
    text += " < ";
    text += expression;
    text += ".end(); ";
    AddSplitMarker(&text);

    text += "++";
    text += node->iterator_->name_;

    // index increment
    if (node->index_ != NULL) {
        text += ", ++";
        text += node->index_->name_;
    }
    
    text += ") {";
    Write(&text, false);
    ++split_level_;
    SynthBlock(node->block_);
}

void CppSynth::SynthForIntRange(AstFor *node)
{
    string  text, aux, top_exp;
    const ExpressionAttributes *attr_low = node->low_->GetAttr();
    const ExpressionAttributes *attr_high = node->high_->GetAttr();
    bool use_top_var = !attr_high->HasKnownValue();
    bool use_step_var = (node->step_value_ == 0);   // is 0 when unknown at compile time (not literal)

    assert(node->iterator_->weak_type_spec_->GetType() == ANT_BASE_TYPE);
    bool using_64_bits = ((AstBaseType*)node->iterator_->weak_type_spec_)->base_type_ == TOKEN_INT64;

    --split_level_;
    text = "for(";
    AddSplitMarker(&text);

    // declaration of iterator
    aux = node->iterator_->name_;
    SynthTypeSpecification(&aux, node->iterator_->weak_type_spec_);
    text += aux;
    text += " = ";
    SynthExpressionAndCastToInt(&aux, node->low_, using_64_bits);
    text += aux;

    // init clause includes declaration of high/step backing variables, index and interator
    if (use_top_var) {
        text += ", ";
        text += node->iterator_->name_;
        text += "__top = ";
        SynthExpressionAndCastToInt(&aux, node->high_, using_64_bits);
        text += aux;
    }
    if (use_step_var) {
        text += ", ";
        text += node->iterator_->name_;
        text += "__step = ";
        SynthExpressionAndCastToInt(&aux, node->step_, using_64_bits);
        text += aux;
    }
    text += "; ";
    AddSplitMarker(&text);

    // end of loop clause.
    if (use_top_var) {
        top_exp = node->iterator_->name_;
        top_exp += "__top";
    } else {
        SynthExpressionAndCastToInt(&top_exp, node->high_, using_64_bits);
    }
    if (use_step_var) {
        text += node->iterator_->name_;
        text += "__step > 0 ? (";
        text += node->iterator_->name_;
        text += " < ";
        text += top_exp;
        text += ") : (";
        text += node->iterator_->name_;
        text += " > ";
        text += top_exp;
        text += "); ";
    } else {
        text += node->iterator_->name_;
        text += node->step_value_ > 0 ? " < " : " > ";
        text += top_exp;
        text += "; ";
    }
    AddSplitMarker(&text);

    // increment clause
    if (node->step_value_ == 1) {
        text += "++";
        text += node->iterator_->name_;
    } else if (node->step_value_ == -1) {
        text += "--";
        text += node->iterator_->name_;
    } else {
        text += node->iterator_->name_;
        text += " += ";
        if (use_step_var) {
            text += node->iterator_->name_;
            text += "__step";
        } else {
            SynthExpressionAndCastToInt(&aux, node->step_, using_64_bits);
            text += aux;
        }
    }
    if (node->index_ != NULL) {
        text += ", ++";
        text += node->index_->name_;
    }

    // close and write down
    text += ") {";
    Write(&text, false);
    ++split_level_;
    SynthBlock(node->block_);
}

void CppSynth::SynthExpressionAndCastToInt(string *dst, IAstExpNode *node, bool use_int64)
{
    int             priority;

    priority = SynthExpression(dst, node);
    Token   target = use_int64 ? TOKEN_INT64 : TOKEN_INT32;
    CastIfNeededTo(target, node->GetAttr()->GetAutoBaseType(), dst, priority, false);
}

/*
types:
- w/o index
- [not] existing iterator/index
- map/vector/string/int iterator
- step

NOTE: iterators are pointers !! (or references)

VarDeclaration  *index_;        // optional
VarDeclaration  *iterator_;
IAstExpNode     *set_;          // if iteration in map/vector/string
IAstExpNode     *low_;          // if int iterator
IAstExpNode     *high_;         // if int iterator
IAstExpNode     *step_;         // if int iterator (optional)
AstBlock        *block_;
*/

void CppSynth::SynthSimpleStatement(AstSimpleStatement *node)
{
    // check: is in an inner block who is continuable/breakable ?

    string text = Lexer::GetTokenString(node->subtype_);
    Write(&text);
}

void CppSynth::SynthReturn(AstReturn *node)
{
    string text;

    if (node->retvalue_ != nullptr) {
        SynthFullExpression(return_type_, &text, node->retvalue_);
        text.insert(0, "return (");
        text += ")";
    } else {
        text = "return";
    }
    Write(&text);
}

int CppSynth::SynthFullExpression(Token target_type, string *dst, IAstExpNode *node)
{
    int             priority;

    priority = SynthExpression(dst, node);
    if (target_type != TOKENS_COUNT) {
        priority = CastIfNeededTo(target_type, node->GetAttr()->GetAutoBaseType(), dst, priority, false);
    }
    return(priority);
}

// TODO: split on more lines
int CppSynth::SynthExpression(string *dst, IAstExpNode *node)
{
    int             priority = 0;

    switch (node->GetType()) {
    case ANT_INDEXING:
        priority = SynthIndices(dst, (AstIndexing*)node);
        break;
    case ANT_FUNCALL:
        priority = SynthFunCall(dst, (AstFunCall*) node);
        break;
    case ANT_BINOP:
        priority = SynthBinop(dst, (AstBinop*)node);
        break;
    case ANT_UNOP:
        priority = SynthUnop(dst, (AstUnop*)node);
        break;
    case ANT_EXP_LEAF:
        priority = SynthLeaf(dst, (AstExpressionLeaf*)node);
        break;
    }
    return(priority);
}

int CppSynth::SynthIndices(string *dst, AstIndexing *node)
{
    string  expression;

    // TODO: ranges/dyna.

    int exp_pri = SynthExpression(dst, node->indexed_term_);
    Protect(dst, exp_pri, GetUnopCppPriority(TOKEN_SQUARE_OPEN));
    if (node->map_type_ == NULL) {

        // an array
        SynthExpression(&expression, node->lower_value_);
    } else {

        // a map
        SynthFullExpression(GetBaseType(node->map_type_->key_type_), &expression, node->lower_value_);
    }
    *dst += '[';
    *dst += expression;
    *dst += ']';
    return(2);
}

int CppSynth::SynthFunCall(string *dst, AstFunCall *node)
{
    int     ii, priority;
    int     numargs = (int)node->arguments_.size();
    string  expression;
    bool builtin = false;

    if (node->left_term_->GetType() == ANT_BINOP) {
        builtin = ((AstBinop*)node->left_term_)->builtin_ != nullptr;
    }

    --split_level_;
    priority = SynthExpression(dst, node->left_term_);
    if (builtin) {

        // fun call alredy synthesized in SynthDotOp() but is missing the arguments
        dst->erase(dst->length() - 1);  // just delete ')' - reopen the arg list
        bool has_already_args = (*dst)[dst->length() - 1] != '(';
        if (has_already_args && numargs > 0) {
            // seperate the two groups 
            *dst += ", ";
        }
        if (has_already_args || numargs > 0) {
            AddSplitMarker(dst);    // has at least an arg.
        }
    } else {
        Protect(dst, priority, GetUnopCppPriority(TOKEN_ROUND_OPEN));
        *dst += '(';
        if (numargs > 0) {
            AddSplitMarker(dst);    // has at least an arg.
        }
    }
    for (ii = 0; ii < numargs; ++ii) {
        VarDeclaration *var = node->func_type_->arguments_[ii];
        Token var_type = GetBaseType(var->weak_type_spec_);
        expression = "";
        SynthFullExpression(var_type, &expression, node->arguments_[ii]->expression_);
        if (!var->HasOneOfFlags(VF_READONLY) && GetParameterPassingMethod(var->weak_type_spec_, false) != PPM_REF) {
            // passed by pointer: get the address (or simplify *)
            if (expression[0] == '*') {
                expression.erase(0, 1);
            } else {
                expression.insert(0, "&");
            }
        }
        if (ii != 0) {
            *dst += ", ";
            AddSplitMarker(dst);
        }
        *dst += expression;
    }
    *dst += ')';
    ++split_level_;
    return(GetUnopCppPriority(TOKEN_ROUND_OPEN));
}

int CppSynth::SynthBinop(string *dst, AstBinop *node)
{
    int priority = 0;

    switch (node->subtype_) {
    case TOKEN_DOT:
        priority = SynthDotOperator(dst, node);
        break;
    case TOKEN_POWER:
        priority = SynthPowerOperator(dst, node);
        break;
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MOD:
    case TOKEN_SHR:
    case TOKEN_AND:
    case TOKEN_SHL:
    case TOKEN_OR:
    case TOKEN_XOR:
        --split_level_;
        priority = SynthMathOperator(dst, node);
        ++split_level_;
        break;
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_GTE:
    case TOKEN_LTE:
    case TOKEN_DIFFERENT:
    case TOKEN_EQUAL:
        --split_level_;
        priority = SynthRelationalOperator(dst, node);
        ++split_level_;
        break;
    case TOKEN_LOGICAL_AND:
    case TOKEN_LOGICAL_OR:
        --split_level_;
        priority = SynthLogicalOperator(dst, node);
        ++split_level_;
        break;
    default:
        // ops !!
        assert(false);
        break;
    }
    return(priority);
}

int CppSynth::SynthDotOperator(string *dst, AstBinop *node)
{
    int priority = GetBinopCppPriority(TOKEN_DOT);
    assert(node->operand_right_->GetType() == ANT_EXP_LEAF);
    AstExpressionLeaf* right_leaf = (AstExpressionLeaf*)node->operand_right_;

    // is a package resolution operator ?
    int pkg_index = -1;
    if (node->operand_left_->GetType() == ANT_EXP_LEAF) {
        AstExpressionLeaf* left_leaf = (AstExpressionLeaf*)node->operand_left_;
        pkg_index = left_leaf->pkg_index_;
        if (pkg_index >= 0) {
            GetFullExternName(dst, pkg_index, right_leaf->value_.c_str());
            return(KLeafPriority);
        } else if (left_leaf->subtype_ == TOKEN_THIS) {
            if (!right_leaf->unambiguous_member_access) {
                *dst = "this->";
            }
            AppendMemberName(dst, right_leaf->wp_decl_);
            return(priority);
        }
    }
    priority = SynthExpression(dst, node->operand_left_);
    const ExpressionAttributes *left_attr = node->operand_left_->GetAttr();
    if (left_attr->IsEnum()) {
        *dst += "::";
        *dst += right_leaf->value_;
        priority = KLeafPriority;
    } else if (node->builtin_ != nullptr) {   
        if (left_attr->IsPointer()) {
            Protect(dst,  priority, GetUnopCppPriority(TOKEN_MPY));
            dst->insert(0, "*");
            priority = GetUnopCppPriority(TOKEN_MPY);
        }
        switch (node->builtin_mode_) {
        case BInSynthMode::sing:
            dst->insert(0, "(");
            dst->insert(0, right_leaf->value_);
            dst->insert(0, "sing::");
            (*dst) += ")";
            priority = GetUnopCppPriority(TOKEN_ROUND_OPEN);
            break;
        case BInSynthMode::cast:
        case BInSynthMode::plain:
            {
                dst->insert(0, "(");
                dst->insert(0, right_leaf->value_);
                (*dst) += ")";
                priority = GetUnopCppPriority(TOKEN_ROUND_OPEN);
                Token base_type = node->operand_left_->GetAttr()->GetAutoBaseType();
                if (base_type != TOKEN_FLOAT64 && node->builtin_mode_ != BInSynthMode::plain) {
                    priority = AddCast(dst, priority, GetBaseTypeName(base_type));
                }
            }
            break;
        case BInSynthMode::member:
        default:
            Protect(dst,  priority, GetBinopCppPriority(TOKEN_DOT));
            (*dst) += ".";
            (*dst) += right_leaf->value_;
            (*dst) += "()";
            priority = GetUnopCppPriority(TOKEN_ROUND_OPEN);
            break;            
        }
    } else {
        if (left_attr->IsPointer()) {
            Protect(dst,  priority, GetUnopCppPriority(TOKEN_MPY));
            dst->insert(0, "(*");
            *dst += ").";
        } else { // ANT_CLASS_TYPE
            Protect(dst,  priority, GetBinopCppPriority(TOKEN_DOT));
            *dst += ".";
        }
        AppendMemberName(dst, right_leaf->wp_decl_);
        priority = GetBinopCppPriority(TOKEN_DOT);
    }
    return(priority);
}

int CppSynth::SynthPowerOperator(string *dst, AstBinop *node)
{
    string          right;

    int  priority = GetBinopCppPriority(node->subtype_);
    int left_priority = SynthExpression(dst, node->operand_left_);
    int right_priority = SynthExpression(&right, node->operand_right_);

    const ExpressionAttributes *left_attr = node->operand_left_->GetAttr();
    const ExpressionAttributes *right_attr = node->operand_right_->GetAttr();
    const ExpressionAttributes *result_attr = node->GetAttr();

    const NumericValue *right_value = right_attr->GetValue();

    Token left_type = left_attr->GetAutoBaseType();
    Token right_type = right_attr->GetAutoBaseType();
    Token result_type = result_attr->GetAutoBaseType();

    // pow2 ?
    if (right_attr->HasKnownValue() && !right_value->IsComplex() && right_value->GetDouble() == 2) {
        if (left_type != result_type) {
            left_priority = AddCast(dst, left_priority, GetBaseTypeName(result_type));
        }
        dst->insert(0, "sing::pow2(");
        *dst += ")";
        return(priority);
    } else {
        if (ExpressionAttributes::BinopRequiresNumericConversion(left_attr, right_attr, TOKEN_POWER)) {
            assert(false);  // since we do no authomatic conversion
            if (left_type != result_type) {
                left_priority = AddCast(dst, left_priority, GetBaseTypeName(result_type));
            }
            if (right_type != result_type) {
                right_priority = AddCast(&right, right_priority, GetBaseTypeName(result_type));
            }
        } else {
            left_priority = PromoteToInt32(left_type, dst, left_priority);
        }
        if (result_attr->IsInteger()) {
            dst->insert(0, "sing::pow(");
        } else {
            dst->insert(0, "std::pow(");
        }
        *dst += ", ";
        *dst += right;
        *dst += ")";
    }
    return(priority);
}

int CppSynth::SynthMathOperator(string *dst, AstBinop *node)
{
    string          right;

    const ExpressionAttributes *left_attr = node->operand_left_->GetAttr();
    const ExpressionAttributes *right_attr = node->operand_right_->GetAttr();
    const ExpressionAttributes *result_attr = node->GetAttr();

    Token left_type = left_attr->GetAutoBaseType();
    Token right_type = right_attr->GetAutoBaseType();
    Token result_type = result_attr->GetAutoBaseType();

    if (node->subtype_ == TOKEN_PLUS && (left_type == TOKEN_STRING || right_type == TOKEN_STRING)) {
        string format, parms;

        ProcessStringSumOperand(&format, &parms, node->operand_left_);
        ProcessStringSumOperand(&format, &parms, node->operand_right_);
        if (format != "ss") {
            *dst = "sing::format(\"";
            *dst += format;
            *dst += "\"";
            *dst += parms;
            *dst += ")";
            return(KForcedPriority);
        } else {
            if (IsLiteralString(node->operand_left_) && IsLiteralString(node->operand_right_)) {
                *dst += ((AstExpressionLeaf*)node->operand_left_)->value_;
                *dst += ' ';
                *dst += ((AstExpressionLeaf*)node->operand_right_)->value_;
                return(KForcedPriority);
            }
            // if not a couple of literals, falls through and uses the sum operator.
        }
    }

    int  priority = GetBinopCppPriority(node->subtype_);
    int left_priority = SynthExpression(dst, node->operand_left_);
    int right_priority = SynthExpression(&right, node->operand_right_);

    if (ExpressionAttributes::BinopRequiresNumericConversion(left_attr, right_attr, node->subtype_)) {
        assert(false);  // since we do no authomatic conversion
        if (left_type != result_type) {
            left_priority = CastIfNeededTo(result_type, left_type, dst, left_priority, false);
        }
        if (right_type != result_type) {
            right_priority = CastIfNeededTo(result_type, right_type, &right, right_priority, false);
        }
    }

    // add brackets if needed
    Protect(dst, left_priority, priority);
    Protect(&right, right_priority, priority, true);

    // sinthesize the operation
    if (node->subtype_ == TOKEN_XOR) {
        *dst += " ^ ";
    } else {
        *dst += ' ';
        *dst += Lexer::GetTokenString(node->subtype_);
        *dst += ' ';
    }
    AddSplitMarker(dst);
    *dst += right;
    return(priority);
}

int CppSynth::SynthRelationalOperator(string *dst, AstBinop *node)
{
    return(SynthRelationalOperator3(dst, node->subtype_, node->operand_left_, node->operand_right_));
}

int CppSynth::SynthRelationalOperator3(string *dst, Token subtype, IAstExpNode *operand_left, IAstExpNode *operand_right)
{
    string          right;

    int  priority = GetBinopCppPriority(subtype);
    int left_priority = SynthExpression(dst, operand_left);
    int right_priority = SynthExpression(&right, operand_right);

    const ExpressionAttributes *left_attr = operand_left->GetAttr();
    const ExpressionAttributes *right_attr = operand_right->GetAttr();

    Token left_type = left_attr->GetAutoBaseType();
    Token right_type = right_attr->GetAutoBaseType();

    if (left_attr->IsInteger() && right_attr->IsInteger()) {

        // use special function in case of signed-unsigned comparison
        bool left_is_uint64 = left_type == TOKEN_UINT64;
        bool right_is_uint64 = right_type == TOKEN_UINT64;
        bool left_is_int64 = left_type == TOKEN_INT64;
        bool right_is_int64 = right_type == TOKEN_INT64;
        bool left_is_uint32 = left_type == TOKEN_UINT32;
        bool right_is_uint32 = right_type == TOKEN_UINT32;
        bool left_is_int32 = left_type == TOKEN_INT32 || left_attr->RequiresPromotion();
        bool right_is_int32 = right_type == TOKEN_INT32 || right_attr->RequiresPromotion();

        bool use_function = left_is_uint64 && right_is_int64 || left_is_uint64 && right_is_int32 || left_is_uint32 && right_is_int32;
        bool use_function_swap = right_is_uint64 && left_is_int64 || right_is_uint64 && left_is_int32 || right_is_uint32 && left_is_int32;

        if (use_function) {
            switch (subtype) {
            case TOKEN_ANGLE_OPEN_LT:
                dst->insert(0, "sing::isless(");
                break;
            case TOKEN_ANGLE_CLOSE_GT:
                dst->insert(0, "sing::ismore(");
                break;
            case TOKEN_GTE:
                dst->insert(0, "sing::ismore_eq(");
                break;
            case TOKEN_LTE:
                dst->insert(0, "sing::isless_eq(");
                break;
            case TOKEN_DIFFERENT:
                dst->insert(0, "!sing::iseq(");
                break;
            case TOKEN_EQUAL:
                dst->insert(0, "sing::iseq(");
                break;
            }
            *dst += ", ";
            AddSplitMarker(dst);
            *dst += right;
            *dst += ")";
            return(subtype == TOKEN_DIFFERENT ? GetUnopCppPriority(TOKEN_LOGICAL_NOT) : KForcedPriority);
        }

        if (use_function_swap) {
            switch (subtype) {
            case TOKEN_ANGLE_OPEN_LT:
                right.insert(0, "sing::ismore(");
                break;
            case TOKEN_ANGLE_CLOSE_GT:
                right.insert(0, "sing::isless(");
                break;
            case TOKEN_GTE:
                right.insert(0, "sing::isless_eq(");
                break;
            case TOKEN_LTE:
                right.insert(0, "sing::ismore_eq(");
                break;
            case TOKEN_DIFFERENT:
                right.insert(0, "!sing::iseq(");
                break;
            case TOKEN_EQUAL:
                right.insert(0, "sing::iseq(");
                break;
            }
            right += ", ";
            AddSplitMarker(&right);
            dst->insert(0, right);
            *dst += ")";
            return(subtype == TOKEN_DIFFERENT ? GetUnopCppPriority(TOKEN_LOGICAL_NOT) : KForcedPriority);
        }
    } else if (left_attr->IsNumber()) {
        CastForRelational(left_type, right_type, dst, &right, &left_priority, &right_priority);
    }

    // add brackets if needed
    Protect(dst, left_priority, priority);
    Protect(&right, right_priority, priority, true);

    // sinthesize the operation
    *dst += ' ';
    *dst += Lexer::GetTokenString(subtype);
    *dst += ' ';
    AddSplitMarker(dst);
    *dst += right;
    return(priority);
}

int CppSynth::SynthLogicalOperator(string *dst, AstBinop *node)
{
    string          right;

    int  priority = GetBinopCppPriority(node->subtype_);
    int left_priority = SynthExpression(dst, node->operand_left_);
    int right_priority = SynthExpression(&right, node->operand_right_);

    // add brackets if needed
    Protect(dst, left_priority, priority);
    Protect(&right, right_priority, priority, true);

    // sinthesize the operation
    *dst += ' ';
    *dst += Lexer::GetTokenString(node->subtype_);
    *dst += ' ';
    AddSplitMarker(dst);
    *dst += right;
    return(priority);
}

int CppSynth::SynthUnop(string *dst, AstUnop *node)
{
    int exp_priority, priority;

    if (node->subtype_ == TOKEN_SIZEOF) {
        priority = KForcedPriority;
    } else {
        priority = 3;
    }
    if (node->operand_ != NULL) {
        exp_priority = SynthExpression(dst, node->operand_);
    }

    switch (node->subtype_) {
    case TOKEN_SIZEOF:
        if (node->type_ != NULL) {
            SynthTypeSpecification(dst, node->type_);
        }
        dst->insert(0, "sizeof(");
        *dst += ')';
        break;
    case TOKEN_MINUS:
        Protect(dst, exp_priority, priority);
        dst->insert(0, "-");
        break;
    case TOKEN_NOT:
        Protect(dst, exp_priority, priority);
        dst->insert(0, "~");
        break;
    case TOKEN_AND:
        Protect(dst, exp_priority, priority);
        if ((*dst)[0] == '*') {
            dst->erase(0, 1);
        } else {
            dst->insert(0, "&");
        }
        break;
    case TOKEN_MPY:
        Protect(dst, exp_priority, priority);
        if ((*dst)[0] == '&') {
            dst->erase(0, 1);
        } else {
            dst->insert(0, "*");
        }
        break;
    case TOKEN_PLUS:
    case TOKEN_LOGICAL_NOT:
        Protect(dst, exp_priority, priority);
        dst->insert(0, Lexer::GetTokenString(node->subtype_));
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
        priority = SynthCastToScalar(dst, node, exp_priority);
        break;
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        priority = SynthCastToComplex(dst, node, exp_priority);
        break;
    case TOKEN_STRING:
        priority = SynthCastToString(dst, node);
        break;
    }
    return(priority);
}

int CppSynth::SynthCastToScalar(string *dst, AstUnop *node, int priority)
{
    const ExpressionAttributes *src_attr;
    bool  explicit_cast = true;

    src_attr = node->operand_->GetAttr();
    if (src_attr->HasComplexType()) {
        Protect(dst, priority, GetBinopCppPriority(TOKEN_DOT));
        *dst += ".real()";
        priority = GetBinopCppPriority(TOKEN_DOT);
        if (src_attr->GetAutoBaseType() == TOKEN_COMPLEX64) {
            explicit_cast = node->subtype_ != TOKEN_FLOAT32;
        } else {
            explicit_cast = node->subtype_ != TOKEN_FLOAT64;
        }
    } else if (src_attr->IsString()) {
        switch (node->subtype_) {
        case TOKEN_INT8:
        case TOKEN_INT16:
        case TOKEN_INT32:
        case TOKEN_INT64:
            dst->insert(0, "sing::string2int(");
            *dst += ")";
            priority = KForcedPriority;
            explicit_cast = node->subtype_ != TOKEN_INT64;
            break;
        case TOKEN_UINT8:
        case TOKEN_UINT16:
        case TOKEN_UINT32:
        case TOKEN_UINT64:
            dst->insert(0, "sing::string2uint(");
            *dst += ")";
            priority = KForcedPriority;
            explicit_cast = node->subtype_ != TOKEN_UINT64;
            break;
        case TOKEN_FLOAT32:
        case TOKEN_FLOAT64:
            dst->insert(0, "sing::string2double(");
            *dst += ")";
            priority = KForcedPriority;
            explicit_cast = node->subtype_ != TOKEN_FLOAT64;
            break;
        }
    }
    if (explicit_cast) {
        priority = AddCast(dst, priority, GetBaseTypeName(node->subtype_));
    }
    return(priority);
}

int CppSynth::SynthCastToComplex(string *dst, AstUnop *node, int priority)
{
    const ExpressionAttributes *src_attr;

    src_attr = node->operand_->GetAttr();
    if (src_attr->HasComplexType()) {
        Token src_type = src_attr->GetAutoBaseType();
        if (src_attr->GetAutoBaseType() == TOKEN_COMPLEX64 && node->subtype_ == TOKEN_COMPLEX128) {
            dst->insert(0, "sing::c_f2d(");
        } else if (src_attr->GetAutoBaseType() == TOKEN_COMPLEX128 && node->subtype_ == TOKEN_COMPLEX64) {
            dst->insert(0, "sing::c_d2f(");
        }
        *dst += ")";
    } else if (src_attr->IsString()) {
        if (node->subtype_ == TOKEN_COMPLEX128) {
            dst->insert(0, "sing::string2complex128(");
        } else {
            dst->insert(0, "sing::string2complex64(");
        }
        *dst += ")";
        priority = KForcedPriority;
    } else {
        Token src_type = src_attr->GetAutoBaseType();
        if (node->subtype_ == TOKEN_COMPLEX128) {
            if (src_type == TOKEN_INT64 || src_type == TOKEN_UINT64) {
                priority = AddCast(dst, priority, "double");
            }
            priority = AddCast(dst, priority, "std::complex<double>");
        } else {
            if (src_type == TOKEN_INT64 || src_type == TOKEN_UINT64 || 
                src_type == TOKEN_INT32 || src_type == TOKEN_UINT32) {
                priority = AddCast(dst, priority, "float");
            }
            priority = AddCast(dst, priority, "std::complex<float>");
        }
    }
    return(priority);
}

int CppSynth::SynthCastToString(string *dst, AstUnop *node)
{
    dst->insert(0, "sing::tostring(");
    *dst += ")";
    return(KForcedPriority);
}

void CppSynth::ProcessStringSumOperand(string *format, string *parms, IAstExpNode *node)
{
    string          operand;

    if (node->GetType() == ANT_UNOP && ((AstUnop*)node)->subtype_ == TOKEN_STRING) {

        // CASE 1: a conversion. generate a type specifier based on the underlying type
        IAstExpNode *child = ((AstUnop*)node)->operand_;
        Token basetype = child->GetAttr()->GetAutoBaseType();
        switch (basetype) {
        case TOKEN_INT8: 
        case TOKEN_INT16:
        case TOKEN_INT32:
            *format += 'd';
            break;
        case TOKEN_INT64:
            *format += 'D';
            break;
        case TOKEN_UINT8:
        case TOKEN_UINT16:
        case TOKEN_UINT32:
            *format += 'u';
            break;
        case TOKEN_UINT64:
            *format += 'U';
            break;
        case TOKEN_FLOAT32:
        case TOKEN_FLOAT64:
            *format += 'f';
            break;
        case TOKEN_COMPLEX64:
            *format += 'r';
            break;
        case TOKEN_COMPLEX128:
            *format += 'R';
            break;
        case TOKEN_BOOL:
            *format += 'b';
            break;
        case TOKEN_STRING:
            *format += 's';
            break;
        default:
            assert(false);
        }
        SynthExpression(&operand, child);
    } else {
        if (node->GetType() == ANT_BINOP) {
            IAstExpNode *node_left  = ((AstBinop*)node)->operand_left_;
            IAstExpNode *node_right = ((AstBinop*)node)->operand_right_;
            Token left_type = node_left->GetAttr()->GetAutoBaseType();
            Token right_type = node_right->GetAttr()->GetAutoBaseType();
            if (((AstBinop*)node)->subtype_ == TOKEN_PLUS && (left_type == TOKEN_STRING || right_type == TOKEN_STRING)) {

                // CASE 2: a sum of strings. recur
                ProcessStringSumOperand(format, parms, node_left);
                ProcessStringSumOperand(format, parms, node_right);
                return;
            }
            assert(false);
        }

        // CASE 3: something we can directly sum to a string. Another string or a number
        const ExpressionAttributes *attr = node->GetAttr();
        if (attr->HasKnownValue()) {
            *format += 'c';
            SynthFullExpression(TOKEN_UINT32, &operand, node);
        } else {
            bool is_string = false;
            switch (attr->GetAutoBaseType()) {
            case TOKEN_INT8:
            case TOKEN_INT16:
            case TOKEN_INT32:
            case TOKEN_UINT8:
            case TOKEN_UINT16:
            case TOKEN_UINT32:
                *format += 'c';
                break;
            case TOKEN_INT64:
            case TOKEN_UINT64:
                *format += 'C';
                break;
            case TOKEN_STRING:
                *format += 's';
                is_string = true;
                break;
            default:
                assert(false);
            }
            int priority = SynthExpression(&operand, node);
            if (is_string && !IsLiteralString(node)) {
                Protect(&operand, priority, GetBinopCppPriority(TOKEN_DOT));
                operand += ".c_str()";
            }
        }
    }
    *parms += ", ";
    AddSplitMarker(parms);
    *parms += operand;
}

int CppSynth::SynthLeaf(string *dst, AstExpressionLeaf *node)
{
    int priority = KLeafPriority;

    switch (node->subtype_) {
    case TOKEN_NULL:
        *dst = "nullptr";
        break;
    case TOKEN_FALSE:
    case TOKEN_TRUE:
        *dst = Lexer::GetTokenString(node->subtype_);
        break;
    case TOKEN_LITERAL_STRING:
        *dst = node->value_;
        break;
    case TOKEN_INT32:
        priority = GetRealPartOfIntegerLiteral(dst, node, 32);
        break;
    case TOKEN_INT64:
        priority = GetRealPartOfIntegerLiteral(dst, node, 64);
        *dst += "LL";
        break;
    case TOKEN_UINT32:
        GetRealPartOfUnsignedLiteral(dst, node);
        *dst += "U";
        break;
    case TOKEN_UINT64:
        GetRealPartOfUnsignedLiteral(dst, node);
        *dst += "LLU";
        break;
    case TOKEN_FLOAT32:
        priority = GetRealPartOfFloatLiteral(dst, node);
        *dst += "f";
        break;
    case TOKEN_FLOAT64:
        priority = GetRealPartOfFloatLiteral(dst, node);
        break;
    case TOKEN_COMPLEX64:
        SynthComplex64(dst, node);
        break;
    case TOKEN_COMPLEX128:
        SynthComplex128(dst, node);
        break;
    case TOKEN_LITERAL_UINT:
        *dst = node->value_;
        dst->erase_occurrencies_of('_');
        break;
    case TOKEN_LITERAL_FLOAT:
        *dst = node->value_;
        *dst += "f";
        dst->erase_occurrencies_of('_');
        break;
    case TOKEN_LITERAL_IMG:
        GetImgPartOfLiteral(dst, node->value_.c_str(), false, false);
        dst->insert(0, "std::complex<float>(0.0f, ");
        *dst += ')';
        break;
    case TOKEN_THIS:
        *dst = "this";
        break;
    case TOKEN_NAME:
        {
            IAstDeclarationNode *decl = node->wp_decl_;
            bool needs_dereferencing = false;
            if (decl->GetType() == ANT_VAR) {
                needs_dereferencing = VarNeedsDereference((VarDeclaration*)decl);
            }
            if (needs_dereferencing) {
                *dst = "*";
                *dst += node->value_;
                priority = GetUnopCppPriority(TOKEN_MPY);
            } else {
                *dst = node->value_;
            }
        }
        break;
    }
    return(priority);
}

void CppSynth::SynthComplex64(string *dst, AstExpressionLeaf *node)
{
    GetRealPartOfFloatLiteral(dst, node);
    dst->insert(0, "std::complex<float>(");
    if (node->img_value_ == "") {
        *dst += "f, 0.0f)";
    } else {
        string img;

        GetImgPartOfLiteral(&img, node->img_value_.c_str(), false, node->img_is_negated_);
        *dst += "f, ";
        *dst += img;
        *dst += ')';
    }
}

void CppSynth::SynthComplex128(string *dst, AstExpressionLeaf *node)
{
    GetRealPartOfFloatLiteral(dst, node);
    dst->insert(0, "std::complex<double>(");
    if (node->img_value_ == "") {
        *dst += ", 0.0)";
    } else {
        string img;

        GetImgPartOfLiteral(&img, node->img_value_.c_str(), node->subtype_ == TOKEN_COMPLEX128, node->img_is_negated_);
        *dst += ", ";
        *dst += img;
        *dst += ')';
    }
}

int CppSynth::GetRealPartOfIntegerLiteral(string *dst, AstExpressionLeaf *node, int nbits)
{
    int64_t value;
    int     priority = KLeafPriority;

    node->GetAttr()->GetSignedIntegerValue(&value);
    if (node->real_is_int_) {

        // keep the original format (es. hex..)
        *dst = node->value_;
        dst->erase_occurrencies_of('_');
        if (node->real_is_negated_) {
            dst->insert(0, "-");
        }
    } else {
        char buffer[100];

        // must have an integer value. Use it and discard the original floating point representation.
        sprintf(buffer, "%lld", value);
        *dst = buffer;
    }
    if (nbits == 32 && value == -(int64_t)0x80000000) {
        dst->insert(0, "(int32_t)");
        *dst += "LL";
        priority = KCastPriority;
    } else if ((*dst)[0] == '-') {
        priority = GetUnopCppPriority(TOKEN_MINUS);
    }
    return(priority);
}

void CppSynth::GetRealPartOfUnsignedLiteral(string *dst, AstExpressionLeaf *node)
{
    if (node->real_is_int_) {

        // keep the original format (es. hex..)
        *dst = node->value_;
        dst->erase_occurrencies_of('_');
    } else {
        uint64_t value;
        char buffer[100];

        // must have an integer value. Use it and discard the original floating point representation.
        value = node->GetAttr()->GetUnsignedValue();
        sprintf(buffer, "%llu", value);
        *dst = buffer;
    }
}

int CppSynth::GetRealPartOfFloatLiteral(string *dst, AstExpressionLeaf *node)
{
    if (IsFloatFormat(node->value_.c_str())) {
        *dst = node->value_;
        dst->erase_occurrencies_of('_');
        if (node->real_is_negated_) {
            dst->insert(0, "-");
        }
    } else {
        int64_t value;
        char buffer[100];

        // must have an integer value. Use it and convert to a floating point representation.
        value = (int64_t)node->GetAttr()->GetDoubleValue();
        sprintf(buffer, "%lld.0", value);
        *dst = buffer;
    }
    if ((*dst)[0] == '-') {
        return(GetUnopCppPriority(TOKEN_MINUS));
    }
    return(KLeafPriority);
}

void CppSynth::GetImgPartOfLiteral(string *dst, const char *src, bool is_double, bool is_negated)
{
    *dst = src;
    dst->erase_occurrencies_of('_');
    dst->erase(dst->length() - 1);      // erase 'i' 
    if (!IsFloatFormat(src)) {
        *dst += ".0";
    }
    if (!is_double) {
        *dst += "f";
    }
    if (is_negated) {
        dst->insert(0, "-");
    }
}

bool IsFloatFormat(const char *num)
{
    return (strchr(num, 'e') != nullptr || strchr(num, 'E') != nullptr || strchr(num, '.') != nullptr);
}

void CppSynth::Write(string *text, bool add_semicolon)
{
    if (add_semicolon) {
        *text += ';';
    }

    // adds indentation and line feed, 
    // if appropriate splits the line at the split markers,
    // adds comments
    formatter_.Format(text, indent_);
    fwrite(formatter_.GetString(), formatter_.GetLength(), 1, file_);
}

void CppSynth::AddSplitMarker(string *dst)
{
    *dst += MAX(split_level_, 0xf8);
}

void  CppSynth::SetFormatterRemarks(IAstNode *node)
{
    formatter_.FormatResidualRemarks();
    int remlen = formatter_.GetLength();
    if (remlen > 0) {
        fwrite(formatter_.GetString(), remlen, 1, file_);
    }
    if (node == nullptr) {
        formatter_.SetRemarks(nullptr, 0);
    } else {
        PositionInfo *pos = node->GetPositionRecord();
        formatter_.SetRemarks(&root_->remarks_[pos->first_remark], pos->num_remarks);
    }
}

void CppSynth::EmptyLine(void)
{
    formatter_.AddLineBreak();
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
        return("std::complex<float>");
    case TOKEN_COMPLEX128:
        return("std::complex<double>");
    case TOKEN_STRING:
        return("sing::string");
    case TOKEN_BOOL:
        return("bool");
    case TOKEN_ERRORCODE:
        return("int32_t");
    case TOKEN_VOID:
        return("void");
    }
    return("");
}

int  CppSynth::GetBinopCppPriority(Token token)
{
    switch (token) {
    case TOKEN_POWER:
        return(KForcedPriority);      // means "doesn't require parenthesys/doesn't take precedence over childrens."
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
    default:
        assert(false);
        break;
    }
    return(KForcedPriority);
}

int  CppSynth::GetUnopCppPriority(Token token)
{
    switch (token) {
    case TOKEN_SQUARE_OPEN: // subscript
    case TOKEN_ROUND_OPEN:  // function call
    case TOKEN_INC:
    case TOKEN_DEC:
    case TOKEN_DOT:
        return(2);
    case TOKEN_SIZEOF:
        return(KForcedPriority);
    case TOKEN_MINUS:
    case TOKEN_PLUS:
    case TOKEN_NOT:         // bitwise not
    case TOKEN_AND:         // address
    case TOKEN_LOGICAL_NOT:
    case TOKEN_MPY:         // dereference
    default:                // casts
        assert(false);
        break;
    }
    return(3);
}

bool CppSynth::VarNeedsDereference(VarDeclaration *var)
{
    if (var->HasOneOfFlags(VF_ISPOINTED | VF_IS_REFERENCE)) return(true);
    if (var->HasOneOfFlags(VF_ISARG)) {
        // output and not a vector
        return (!var->HasOneOfFlags(VF_READONLY) && GetParameterPassingMethod(var->weak_type_spec_, false) != PPM_REF);
    }
    return(false);
}

void CppSynth::PrependWithSeparator(string *dst, const char *src)
{
    if (dst->length() == 0) {
        *dst = src;
    } else {
        dst->insert(0, src);
        dst->insert(strlen(src), " ");
    }
}

int CppSynth::AddCast(string *dst, int priority, const char *cast_type) {
    char prefix[100];

    // special case: must cast a single literal
    //if (priority == KLiteralPriority) {
    //    if (strcmp(cast_type, "uint64_t") == 0) {
    //        CutDecimalPortionAndSuffix(dst);
    //        *dst += "ull";
    //        return(KLeafPriority);                  // so that this thick is used just once
    //    } else if (strcmp(cast_type, "int64_t") == 0) {
    //        CutDecimalPortionAndSuffix(dst);
    //        *dst += "ll";
    //        return(KLeafPriority);
    //    } else if (strcmp(cast_type, "uint32_t") == 0) {
    //        CutDecimalPortionAndSuffix(dst);
    //        *dst += "u";
    //        return(KLeafPriority);
    //    } else if (strcmp(cast_type, "int32_t") == 0) {
    //        CutDecimalPortionAndSuffix(dst);
    //        return(KLeafPriority);
    //    } else if (strcmp(cast_type, "float") == 0) {
    //        CutSuffix(dst);
    //        if (dst->find('.') == string::npos) {
    //            *dst += ".0f";
    //        } else {
    //            *dst += "f";
    //        }
    //        return(KLeafPriority);
    //    } else if (strcmp(cast_type, "double") == 0) {
    //        CutSuffix(dst);
    //        *dst += ".0";
    //        return(KLeafPriority);
    //    }
    //    // if casting to a different type than the ones above, fallthrough to the standard case.
    //}

    if (cast_type == nullptr || cast_type[0] == 0) {
        return(priority);
    }
    if (priority > KCastPriority) {
        sprintf(prefix, "(%s)(", cast_type);
        dst->insert(0, prefix);
        *dst += ")";
    } else {
        sprintf(prefix, "(%s)", cast_type);
        dst->insert(0, prefix);
    }
    return(KCastPriority);
}

void CppSynth::CutDecimalPortionAndSuffix(string *dst)
{
    int cut_point = dst->find('.');
    if (cut_point != string::npos) {
        dst->erase(cut_point);
        return;
    }

    // if '.' was not found this could be an integer with an 'll' suffix
    CutSuffix(dst);
}

void CppSynth::CutSuffix(string *dst)
{
    int ii;
    for (ii = dst->length() - 1; ii > 0 && dst->c_str()[ii] == 'l'; --ii);
    dst->erase(ii + 1);
}

//
// casts numerics if c++ doesn't automatically cast to target
//
int CppSynth::CastIfNeededTo(Token target, Token src_type, string *dst, int priority, bool for_power_op)
{
    if (target == src_type || target == TOKEN_BOOL || target == TOKEN_STRING || target == TOKENS_COUNT) {
        return(priority);
    }
    if (target == TOKEN_COMPLEX128) {
        if (src_type == TOKEN_COMPLEX64) {
            dst->insert(0, "sing::c_f2d(");
            *dst += ")";
            return(KForcedPriority);
        } else if (src_type != TOKEN_COMPLEX128 && src_type != TOKEN_FLOAT64) {
            priority = AddCast(dst, priority, "double");
        }
        return(priority);
    }
    if (target == TOKEN_COMPLEX64) {
        if (src_type == TOKEN_COMPLEX128) {
            dst->insert(0, "sing::c_d2f(");
            *dst += ")";
            return(KForcedPriority);
        } else if (src_type != TOKEN_COMPLEX64 && src_type != TOKEN_FLOAT32) {
            priority = AddCast(dst, priority, "float");
        }
        return(priority);
    }

    // the target is scalar
    if (src_type == TOKEN_COMPLEX128 || src_type == TOKEN_COMPLEX64) {
        Protect(dst, priority, GetBinopCppPriority(TOKEN_DOT));
        *dst += ".real()";
        priority = GetBinopCppPriority(TOKEN_DOT);
        src_type = (src_type == TOKEN_COMPLEX128) ? TOKEN_FLOAT64 : TOKEN_FLOAT32;
    }

    if (ExpressionAttributes::CanAssignWithoutLoss(target, src_type)) {
        return(priority);
    }

    priority = AddCast(dst, priority, GetBaseTypeName(target));
    return(priority);
}

void CppSynth::CastForRelational(Token left_type, Token right_type, string *left, string *right, int *priority_left, int *priority_right)
{
    // complex comparison
    if (left_type == TOKEN_COMPLEX128 || right_type == TOKEN_COMPLEX128 ||
        left_type == TOKEN_COMPLEX64 && right_type == TOKEN_FLOAT64 ||
        right_type == TOKEN_COMPLEX64 && left_type == TOKEN_FLOAT64) {
        *priority_left = CastIfNeededTo(TOKEN_COMPLEX128, left_type, left, *priority_left, false);
        *priority_right = CastIfNeededTo(TOKEN_COMPLEX128, right_type, right, *priority_right, false);
    } else if (left_type == TOKEN_COMPLEX64 || right_type == TOKEN_COMPLEX64) {
        *priority_left = CastIfNeededTo(TOKEN_COMPLEX64, left_type, left, *priority_left, false);
        *priority_right = CastIfNeededTo(TOKEN_COMPLEX64, right_type, right, *priority_right, false);
    } else if (left_type == TOKEN_FLOAT64 || right_type == TOKEN_FLOAT64) {
        *priority_left = CastIfNeededTo(TOKEN_FLOAT64, left_type, left, *priority_left, false);
        *priority_right = CastIfNeededTo(TOKEN_FLOAT64, right_type, right, *priority_right, false);
    } else {
        // we are sure than not both the values are integers, so we know that at least one is float
        *priority_left = CastIfNeededTo(TOKEN_FLOAT32, left_type, left, *priority_left, false);
        *priority_right = CastIfNeededTo(TOKEN_FLOAT32, right_type, right, *priority_right, false);
    }
}

int CppSynth::PromoteToInt32(Token target, string *dst, int priority)
{
    switch (target) {
    case TOKEN_INT16:
    case TOKEN_INT8:
    case TOKEN_UINT16:
    case TOKEN_UINT8:
        return(AddCast(dst, priority, "int32_t"));
    default:
        break;
    }
    return(priority);
}

// adds brackets if adding the next operator would invert the priority 
// to be done to operands after casts and before operation 
void CppSynth::Protect(string *dst, int priority, int next_priority, bool is_right_term) {

    // a function-like operator: needs no protection and causes no inversion
    if (priority == KForcedPriority || next_priority == KForcedPriority) return;

    // protect the priority of this branch from adjacent operators
    // note: if two binary operators have same priority, use brackets if right-associativity is required
    if (next_priority < priority  || is_right_term && priority > 3 && next_priority == priority) {
        dst->insert(0, "(");
        *dst += ')';
    }
}

bool CppSynth::IsPOD(IAstTypeNode *node)
{
    bool ispod = true;

    switch (node->GetType()) {
    case ANT_BASE_TYPE:
        switch (((AstBaseType*)node)->base_type_) {
        case TOKEN_COMPLEX64:
        case TOKEN_COMPLEX128:
        case TOKEN_STRING:
            ispod = false;
        default:
            break;
        }
        break;
    case ANT_NAMED_TYPE:
        ispod = IsPOD(((AstNamedType*)node)->wp_decl_->type_spec_);
        break;
    case ANT_ARRAY_TYPE:
    case ANT_MAP_TYPE:
    case ANT_POINTER_TYPE:
        ispod = false;
        break;
    case ANT_FUNC_TYPE:
    case ANT_ENUM_TYPE:
    default:
        break;
    }
    return(ispod);
}

Token CppSynth::GetBaseType(IAstTypeNode *node)
{
    switch (node->GetType()) {
    case ANT_BASE_TYPE:
        return(((AstBaseType*)node)->base_type_);
    case ANT_NAMED_TYPE:
        return(GetBaseType(((AstNamedType*)node)->wp_decl_->type_spec_));
    default:
        break;
    }
    return(TOKENS_COUNT);
}

void CppSynth::GetFullExternName(string *full, int pkg_index, const char *local_name)
{
    assert(pkg_index >= 0 && pkg_index < (int)packages_->size());
    if (pkg_index >= 0 && pkg_index < (int)packages_->size()) {
        string *nspace = &(*packages_)[pkg_index]->root_->namespace_;
        if (root_->namespace_ != nspace) {
            const char *src = nspace->c_str();

            (*full) = "";
            for (int ii = 0; ii < (int)nspace->length(); ++ii) {
                if (src[ii] != '.') {
                    (*full) += src[ii];
                } else {
                    (*full) += "::";
                }
            }
            (*full) += "::";
            (*full) += local_name;
        } else {
            (*full) = local_name;
        }
    } else {
        (*full) = local_name;
    }
}

bool CppSynth::IsLiteralString(IAstExpNode *node)
{
    if (node->GetType() == ANT_EXP_LEAF) {
        AstExpressionLeaf *leaf = (AstExpressionLeaf*)node;
        return (leaf->subtype_ == TOKEN_LITERAL_STRING);
    }
    return(false);
}

int CppSynth::WriteHeaders(DependencyUsage usage)
{
    string text;
    int num_items = 0;

    for (int ii = 0; ii < (int)root_->dependencies_.size(); ++ii) {
        AstDependency *dependency = root_->dependencies_[ii];
        if (dependency->GetUsage() == usage) {
            text = dependency->package_dir_.c_str();
            FileName::ExtensionSet(&text, "h");
            text.insert(0, "#include \"");
            text += "\"";
            SetFormatterRemarks(dependency);
            formatter_.SetNodePos(dependency->GetPositionRecord());
            Write(&text, false);
            ++num_items;
        }
    }
    SetFormatterRemarks(nullptr);
    return(num_items);
}

int CppSynth::WriteNamespaceOpening(void)
{
    int num_levels = 0;

    const char *scan = root_->namespace_.c_str();
    if (*scan != 0) {
        string text;

        while (*scan != 0) {
            text = "namespace ";
            while (*scan != '.' && *scan != 0) {
                text += *scan++;
            }
            text += " {";
            Write(&text, false);
            while (*scan == '.') ++scan;
            ++num_levels;
        }
    }
    if (num_levels > 0) {
        EmptyLine();
    }
    return(num_levels);
}

void CppSynth::WriteNamespaceClosing(int num_levels)
{
    if (num_levels > 0) {
        EmptyLine();
        string closing;
        for (int ii = 0; ii < num_levels; ++ii) {
            closing = "}   // namespace";
            Write(&closing, false);
        }
    }
}

void CppSynth::WriteClassForwardDeclarations(bool public_defs)
{
    bool empty_section = true;
    string text;
    ForwardReferenceType reftype = public_defs ? FRT_PUBLIC : FRT_PRIVATE;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            if (tdecl->forward_referral_ != reftype) continue;
            text = "class ";
            text += tdecl->name_;
            Write(&text);
            empty_section = false;
        }
    }
    if (!empty_section) {
        EmptyLine();
    }    
}

int CppSynth::WriteTypeDefinitions(bool public_defs)
{
    int num_items = 0;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->IsPublic() != public_defs) continue;
        switch (declaration->GetType()) {
        case ANT_VAR:
            {
                VarDeclaration *var = (VarDeclaration*)declaration;
                if (var->HasOneOfFlags(VF_INVOLVED_IN_TYPE_DEFINITION)) {
                    SetFormatterRemarks(var);
                    formatter_.SetNodePos(var->GetPositionRecord());
                    SynthVar(var);
                    ++num_items;
                }
            }
            break;
        case ANT_TYPE:
            SetFormatterRemarks(declaration);
            formatter_.SetNodePos(declaration->GetPositionRecord());
            SynthType((TypeDeclaration*)declaration);
            ++num_items;
            break;
        default:
            break;
        }
    }
    if (num_items > 0) {
        EmptyLine();
    }
    SetFormatterRemarks(nullptr);
    return(num_items);
}

void CppSynth::WritePrototypes(bool public_defs)
{
    bool empty_section = true;
    string text;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->IsPublic() != public_defs) continue;
        if (declaration->GetType() == ANT_FUNC) {
            FuncDeclaration *func = (FuncDeclaration*)declaration;
            if (!func->is_class_member_) {
                text = func->name_;
                SynthFuncTypeSpecification(&text, func->function_type_, true);
                if (!public_defs) {
                    text.insert(0, "static ");
                }
                Write(&text);
                empty_section = false;
            }
        }
    }
    if (!empty_section) {
        EmptyLine();
    }
}

void CppSynth::WriteExternalDeclarations(void)
{
    bool empty_section = true;
    string text;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (!declaration->IsPublic()) continue;
        if (declaration->GetType() == ANT_VAR) {
            VarDeclaration *var = (VarDeclaration*)declaration;
            if (!var->HasOneOfFlags(VF_INVOLVED_IN_TYPE_DEFINITION)) {
                text = var->name_;
                SynthTypeSpecification(&text, var->weak_type_spec_);
                text.insert(0, "extern const ");
                Write(&text);
                empty_section = false;
            }
        }
    }
    if (!empty_section) {
        EmptyLine();
    }
}

int CppSynth::WriteVariablesDefinitions(void)
{
    int num_items = 0;
    string text;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_VAR) {
            VarDeclaration *var = (VarDeclaration*)declaration;
            if (!var->HasOneOfFlags(VF_INVOLVED_IN_TYPE_DEFINITION)) {
                SetFormatterRemarks(declaration);
                formatter_.SetNodePos(declaration->GetPositionRecord());
                SynthVar((VarDeclaration*)declaration);
                ++num_items;
            }
        }
    }
    if (num_items > 0) {
        EmptyLine();
    }
    SetFormatterRemarks(nullptr);
    return(num_items);
}

int CppSynth::WriteClassIdsDefinitions(void)
{
    int num_items = 0;
    string text;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            bool towrite = false;

            // if has base classes, downcast is possible. 
            if (tdecl->type_spec_->GetType() == ANT_CLASS_TYPE) {
                AstClassType *ctype = (AstClassType*)tdecl->type_spec_;    
                towrite = ctype->member_interfaces_.size() > 0;
            }
            if (towrite) {
                text = "char ";
                text += tdecl->name_;
                text += "::id__";
                Write(&text);
                ++num_items;
            }
        }
    }
    if (num_items > 0) {
        EmptyLine();
    }
    SetFormatterRemarks(nullptr);
    return(num_items);     
}

int CppSynth::WriteConstructors(void)
{
    int num_items = 0;
    string text;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            if (tdecl->type_spec_->GetType() == ANT_CLASS_TYPE) {
                AstClassType *ctype = (AstClassType*)tdecl->type_spec_;    

                // note: if the class has functions, the constructor is placed just before the first of them            
                if (ctype->has_constructor && !ctype->constructor_written && ctype->member_functions_.size() == 0) {
                    ++num_items;
                    SynthConstructor(&tdecl->name_, ctype);
                }
            }
        }
    }
    if (num_items > 0) {
        EmptyLine();
    }
    SetFormatterRemarks(nullptr);
    return(num_items); 
}

int CppSynth::WriteFunctions(void)
{
    string text;
    int num_items = 0;

    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_FUNC) {
            SetFormatterRemarks(declaration);
            formatter_.SetNodePos(declaration->GetPositionRecord());
            SynthFunc((FuncDeclaration*)declaration);
            ++num_items;
        }
    }
    SetFormatterRemarks(nullptr);
    return(num_items);
}

AstClassType *CppSynth::GetLocalClassTypeDeclaration(const char *classname)
{
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            if (tdecl->name_ == classname) {
                IAstTypeNode *ntype = tdecl->type_spec_;
                if (ntype != nullptr && ntype->GetType() == ANT_CLASS_TYPE) {
                    return((AstClassType*)ntype);
                }
                return(nullptr);
            }
        }
    }    
    return(nullptr);
}

void CppSynth::AppendMemberName(string *dst, IAstDeclarationNode *src)
{
    assert(src != nullptr);
    if (src == nullptr) return;
    if (src->GetType() == ANT_VAR) {
        VarDeclaration *var = (VarDeclaration*)src;
        *dst += member_prefix_;
        *dst += var->name_;
        *dst += member_suffix_;
    } else if (src->GetType() == ANT_FUNC) {
        FuncDeclaration *fun = (FuncDeclaration*)src;
        *dst += fun->name_;
    } else {
        assert(false);
    }
}

void CppSynth::SynthDFile(FILE *dfd, Package *package, const char *target_name)
{
    fprintf(dfd, "%s:", target_name);
    vector<AstDependency*> *vdep = &package->root_->dependencies_;
    for (int ii = 0; ii < vdep->size(); ++ii) {
        AstDependency *dep = (*vdep)[ii];
        if (ii == vdep->size() - 1) {
            fprintf(dfd, " %s", dep->full_package_path_.c_str());
        } else {
            fprintf(dfd, " %s \\", dep->full_package_path_.c_str());
        }
    }
}

void CppSynth::SynthMapFile(FILE *mfd)
{
    fprintf(mfd, "prefix = %s\r\n", member_prefix_.c_str());
    fprintf(mfd, "suffix = %s\r\n", member_suffix_.c_str());
    const vector<line_nums> *lines = formatter_.GetLines();
    int top = lines->size() - 1;
    if (top < 0) {
        fprintf(mfd, "top_lines = 0, 0\r\n");
    } else {
        fprintf(mfd, "top_lines = %d, %d\r\n", (*lines)[top].sing_line, (*lines)[top].cpp_line);
        fprintf(mfd, "lines:\r\n");
        for (int ii = 0; ii <= top; ++ii) {
            fprintf(mfd, "%d, %d\r\n", (*lines)[ii].sing_line, (*lines)[ii].cpp_line);
        }
    }
}

} // namespace

