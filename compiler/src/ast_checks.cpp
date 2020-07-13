#include <assert.h>
#include <float.h>
#include <math.h>
#include "ast_checks.h"
#include "target.h"
#include "builtin_functions.h"

namespace SingNames {

bool AstChecker::CheckAll(vector<Package*> *packages, Options *options, int pkg_index, bool fully_parsed)
{
    packages_ = packages;
    Package *pkg = (*packages)[pkg_index];
    root_ = pkg->root_;
    errors_ = &pkg->errors_;
    symbols_ = &pkg->symbols_;
    options_ = options;
    in_function_block_ = false;
    in_class_declaration_ = false;
    current_function_ = nullptr;
    current_class_ = nullptr;
    usage_errors_.Reset();
    check_usage_errors_ = fully_parsed && options_->AreUsageErrorsEnabled();

    for (int ii = 0; ii < (int)root_->dependencies_.size(); ++ii) {
        AstDependency *dependency = root_->dependencies_[ii];
        bool on_error = true;

        // solve the name
        switch (options->SolveFileName(nullptr, &dependency->full_package_path_, dependency->package_dir_.c_str())) {
        case FileSolveError::AMBIGUOUS:
            Error("Ambiguous path/filename: can be found in multiple search directories", dependency);
            break;
        case FileSolveError::NOT_FOUND:
            Error("Invalid path", dependency);
            break;
        case FileSolveError::OK:
            on_error = false;
            break;
        default:
            break;
        }
        
        // duplicated ?
        if (!on_error) {
            for (int jj = 0; jj < ii; ++jj) {
                AstDependency *existent = root_->dependencies_[jj];
                if (is_same_filename(existent->full_package_path_.c_str(), dependency->full_package_path_.c_str())) {
                    string errmess = "Duplicated 'requires' clause. After search directory resolution, both clauses map to: ";
                    errmess += dependency->full_package_path_;
                    Error(errmess.c_str(), dependency);
                    on_error = true;
                    break;
                }
                if (existent->package_name_ == dependency->package_name_) {
                    string errmess = "Two 'requires' clauses share the same local name: ";
                    errmess += dependency->package_name_;
                    Error(errmess.c_str(), dependency);
                    dependency->ambiguous_ = true;
                    existent->ambiguous_ = true;
                    on_error = true;
                    break;
                }
            }
        }

        // note: we check for inclusion cycles including the compiled file. When we compile other files we check for other loops.
        if (!on_error && is_same_file((*packages)[0]->fullpath_.c_str(), dependency->full_package_path_.c_str())) {
            Error("Cyclic inclusions are disallowed", dependency);
            dependency->package_index_ = 0;
            on_error = true;
        }

        // if already loaded set the index, even if on error.  
        if (dependency->package_index_ == -1) {
            for (int ii = 1; ii < (int)packages_->size(); ++ii) {
                if (is_same_filename((*packages)[ii]->fullpath_.c_str(), dependency->full_package_path_.c_str())) {
                    dependency->package_index_ = ii;
                    break;
                }
            }
        }

        // if not found
        if (dependency->package_index_ == -1) {
            dependency->package_index_ = (int)packages_->size();
            Package *pkg = new Package;
            pkg->Init(dependency->full_package_path_.c_str());
            if (on_error) {
                pkg->SetError();
            }
            packages_->push_back(pkg);
        }
    }

    // all the declarations
    loops_nesting = 0;
    for (current_ = 0; current_ < (int)root_->declarations_.size(); ++current_) {      
        IAstDeclarationNode *declaration = root_->declarations_[current_];
        current_is_public_ = declaration->IsPublic();
        switch (declaration->GetType()) {
        case ANT_VAR:
            CheckVar((VarDeclaration*)declaration);
            break;
        case ANT_TYPE:
            CheckType((TypeDeclaration*)declaration);
            break;
        case ANT_FUNC:
            CheckFunc((FuncDeclaration*)declaration);
            break;
        default:
            assert(false);
        }
    }

    // all the bodies
    for (current_ = 0; current_ < (int)root_->declarations_.size(); ++current_) {
        IAstDeclarationNode *declaration = root_->declarations_[current_];
        current_is_public_ = declaration->IsPublic();
        if (declaration->GetType() == ANT_FUNC) {
            CheckFuncBody((FuncDeclaration*)declaration);
        }
    }

    for (int ii = 0; ii < (int)root_->dependencies_.size(); ++ii) {
        AstDependency *dependency = root_->dependencies_[ii];
        if (dependency->GetUsage() == DependencyUsage::UNUSED && fully_parsed) {
            string errormess = "package ";
            errormess += dependency->package_dir_;
            errormess += " has been required but not used.";
            Error(errormess.c_str(), dependency);
        } else {
            int index = dependency->package_index_;
            if (index < (int)packages_->size() && index >= 0) {
                Package *pkg = (*packages_)[index];
                if (pkg->status_ < PkgStatus::FOR_REFERENCIES) {
                    Error("Failed to load package", dependency);
                }
            }
        }
    }

    if (fully_parsed && !options_->GenerateHOnly()) {
        CheckMemberFunctionsDeclarationsPresence();
    }
    
    if (check_usage_errors_ && errors_->NumErrors() == 0) {
        CheckPrivateDeclarationsUsage();
        errors_->Append(&usage_errors_);
    }

    return(errors_->NumErrors() == 0);
}

void AstChecker::CheckVar(VarDeclaration *declaration)
{
    bool check_constness = true;

    // insert now so that we can check conflicts with names in initers
    InsertName(declaration->name_.c_str(), declaration);
    
    if (declaration->IsPublic() && !declaration->HasOneOfFlags(VF_READONLY)) {
        Error("Public global variables are forbidden !", declaration);
    }
    if (declaration->type_spec_ != nullptr) {
        if (CheckTypeSpecification(declaration->type_spec_, declaration->initer_ != nullptr ? TSCM_INITEDVAR : TSCM_STD)) {
            if (declaration->initer_ != nullptr) {
                if (!CheckIniter(declaration->type_spec_, declaration->initer_)) {
                    check_constness = false;
                }
            }
        }
    } else if (declaration->initer_ == nullptr || declaration->initer_->GetType() == ANT_INITER) {
        Error("Auto variables require a single expression initer", declaration->initer_);
    } else {
        ExpressionAttributes attr;
        CheckExpression((IAstExpNode*)declaration->initer_, &attr, ExpressionUsage::READ);
        if (!attr.IsOnError()) {

            // inited with a variable of arbitrary type ?
            declaration->weak_type_spec_ = attr.GetAutoType();
            if (declaration->weak_type_spec_ == nullptr) {

                // inited with a right expression (a temporary value) of basic type ?
                Token basetype = attr.GetAutoBaseType();
                if (basetype < TOKENS_COUNT) {
                    declaration->SetType(new AstBaseType(basetype));
                } else {

                    // inited with the address of a variable of arbitrary type ?
                    bool writable;
                    IAstTypeNode *pointed_type = attr.GetTypeFromAddressOperator(&writable);
                    if (pointed_type != nullptr) {
                        AstPointerType *typenode = new AstPointerType();
                        typenode->SetWithRef(!writable, pointed_type);
                        declaration->SetType(typenode);
                    } else {

                        // can't derive the type !
                        Error("The initer for an auto variable must be typed", declaration);
                        check_constness = false;
                    }
                }
            }
        } else {
            check_constness = false;
        }
    }

    // if appropriate, set the VF_IMPLEMENTED_AS_CONSTINT flag
    // const with a single initer...
    if (declaration->HasOneOfFlags(VF_READONLY) && declaration->initer_ != nullptr && declaration->initer_->GetType() != ANT_INITER) {

        // ...of type integer and known value...
        IAstExpNode* exp = (IAstExpNode*)declaration->initer_;
        const ExpressionAttributes *attr = exp->GetAttr();
        if ((attr->IsInteger() || attr->IsEnum()) && attr->HasKnownValue()) {

            // ...using only some allowed operators.
            if (VerifyIndexConstness(exp)) {
                declaration->SetFlags(VF_IMPLEMENTED_AS_CONSTINT);
            }
        }
    }
}

void AstChecker::CheckMemberVar(VarDeclaration *declaration)
{
    ExpressionAttributes    attr;
    Token                   basetype;

    if (declaration->initer_ != nullptr) {
        if (declaration->initer_->GetType() == ANT_INITER) {
            Error("Member variables only support single value initers", declaration->initer_);
        } else {
            CheckExpression((IAstExpNode*)declaration->initer_, &attr, ExpressionUsage::READ);
            if (!attr.IsOnError()) {
                basetype = attr.GetAutoBaseType();
                if (basetype == TOKENS_COUNT) {
                    Error("You can initialize only member variables of base type", declaration->initer_);
                    attr.SetError();
                } else if (!IsCompileTimeConstant((IAstExpNode*)declaration->initer_)) {
                    Error("The argument's initer must be a compile time constant !", declaration->initer_);
                    attr.SetError();
                }
            }
        }   
    }
    if (declaration->type_spec_ != nullptr) {
        if (CheckTypeSpecification(declaration->type_spec_, declaration->initer_ != nullptr ? TSCM_INITEDVAR : TSCM_STD)) {
            if (!attr.IsOnError()) {
                ExpressionAttributes dst_attr;

                IAstTypeNode *type_spec = SolveTypedefs(declaration->type_spec_);
                dst_attr.InitWithTree(type_spec, nullptr, true, true, this);
                CanAssign(&dst_attr, &attr, declaration->initer_);
            }
        }
    } else if (!attr.IsOnError()) {
        declaration->SetType(new AstBaseType(basetype));
    }
}

void AstChecker::CheckType(TypeDeclaration *declaration)
{
    if (CheckTypeSpecification(declaration->type_spec_, TSCM_STD)) {
        if (declaration->type_spec_->GetType() == ANT_NAMED_TYPE) {
            Error("You can't have a type declaration whose only purpose is to rename an existing type", declaration->type_spec_);
        }
    }
    InsertName(declaration->name_.c_str(), declaration);
}

void AstChecker::CheckFunc(FuncDeclaration *declaration)
{
    if (declaration->is_class_member_) {
        CheckMemberFunc(declaration);
    } else {
        AstFuncType *functype = declaration->function_type_;

        // the parser ensures this is a function type.
        if (CheckTypeSpecification(functype, TSCM_STD)) {
            declaration->SetOk();
        }
        InsertName(declaration->name_.c_str(), declaration);
    }
}

void AstChecker::CheckMemberFunc(FuncDeclaration *declaration)
{
    if (declaration->block_ == nullptr) {
        Error("You can't omit the function body in a member function declaration", declaration);
        return;
    }

    bool found = false;
    bool isok = CheckTypeSpecification(declaration->function_type_, TSCM_STD);
    AstClassType *ctype = GetLocalClassTypeDeclaration(declaration->classname_.c_str(), false);
    if (ctype != nullptr) {
        for (int ii = 0; ii < (int)ctype->member_functions_.size(); ++ii) {
            if (ctype->member_functions_[ii]->name_ == declaration->name_) {
                found = true;
                FuncDeclaration *func = ctype->member_functions_[ii];
                declaration->SetMuting(func->is_muting_);
                if (ii >= ctype->first_hinherited_member_) {
                    declaration->SetVirtual(true);
                }
                if (ctype->implemented_[ii]) {
                    Error("Double defined function", declaration);
                    isok = false;
                }
                if (AreTypeTreesCompatible(declaration->function_type_, func->function_type_, FOR_EQUALITY) != OK) {
                    Error("Function signature differs from class declaration", declaration);
                    isok = false;
                }
                if (declaration->IsPublic() && !func->IsPublic()) {
                    Error("Original declaration is not public !", declaration);
                    isok = false;
                }
                if (!func->type_is_ok_) {
                    isok = false;
                }
                ctype->implemented_[ii] = true;
                break;
            }
        }
    }
    if (ctype == nullptr) {
        Error("Can't find the function's class declaration", declaration);
    } else if (!found) {
        Error("Function doesn't belong to the class", declaration);
    } else if (isok) {
        declaration->SetOk();
    }
}

void AstChecker::CheckFuncBody(FuncDeclaration *declaration)
{
    AstFuncType *functype = declaration->function_type_;
    int ii;

    if (!declaration->type_is_ok_ || declaration->block_ == nullptr) {
        return;
    }
    return_fake_variable_.InitWithTree(functype->return_type_, nullptr, true, true, this);
    symbols_->OpenScope();
    for (ii = 0; ii < (int)functype->arguments_.size(); ++ii) {

        // NOTE: already tested in CheckTypeSpecification()
        VarDeclaration *var = functype->arguments_[ii];
        InsertName(var->name_.c_str(), var);
    }

    in_function_block_ = true;
    current_function_ = declaration;
    if (declaration->is_class_member_) {
        current_class_ = GetLocalClassTypeDeclaration(declaration->classname_.c_str(), true);
        this_was_accessed_ = false;
    } else {
        current_class_ = nullptr;
    }

    CheckBlock(declaration->block_, false);

    in_function_block_ = false;
    current_class_ = nullptr;
    current_function_ = nullptr;

    if (declaration->is_class_member_ && !declaration->is_virtual_ && !this_was_accessed_)
    {
        Error("A not inherited member function should access at least a class member", declaration);
    }
    if (!return_fake_variable_.IsVoid()) {
        if (!BlockReturnsExplicitly(declaration->block_)) {
            Error("Function must return a value !!", declaration->block_, true);
        }
    }
}

bool AstChecker::CheckTypeSpecification(IAstNode *type_spec, TypeSpecCheckMode mode)
{
    bool        success = true;
    AstNodeType node_type = type_spec->GetType();

    switch (node_type) {
    case ANT_BASE_TYPE:
        if (mode != TSCM_RETVALUE && ((AstBaseType*)type_spec)->base_type_ == TOKEN_VOID) {
            Error("Void only allowed as a function return type", type_spec);
            success = false;
        }
        break;
    case ANT_NAMED_TYPE:
        {
            IAstDeclarationNode *decl = nullptr;
            AstNamedType *node = (AstNamedType*)type_spec;
            const char *name = node->name_.c_str();
            AstNamedType *second_node = node->next_component;
            if (second_node != nullptr) {
                const char *second_name = second_node->name_.c_str();
                if (second_node->next_component != nullptr) {
                    Error("Qualified type names can have at most 2 components: a package name + a type name", second_node->next_component);
                    success = false;
                } else {
                    int pkg = SearchAndLoadPackage(name, node, "Unknown package name");
                    if (pkg == -1) {
                        success = false;
                    } else {
                        node->pkg_index_ = pkg;
                        bool is_private;
                        decl = SearchExternDeclaration(pkg, second_name, &is_private);
                        if (is_private) {
                            Error("Cannot access private symbol", second_node);
                            success = false;
                        }
                    }
                }
            } else {
                decl = SearchDeclaration(name, node);
                if (decl == nullptr && mode == TSCM_REFERENCED && in_class_declaration_) {
                    decl = ForwardSearchDeclaration(name, node);
                }
            }
            if (success) {
                AstNamedType *error_locus = second_node != nullptr ? second_node : node;
                if (decl == nullptr) {
                    Error("Undefined type", error_locus);
                    success = false;
                } else if (decl->GetType() != ANT_TYPE) {
                    Error("Expected a type name", error_locus);
                    success = false;
                } else if (mode != TSCM_REFERENCED && !NodeIsConcrete(((TypeDeclaration*)decl)->type_spec_)) {
                    Error("An Interface type is not allowed here (only concrete types)", error_locus);
                    success = false;
                } else {
                    node->wp_decl_ = (TypeDeclaration*)decl;
                    ((TypeDeclaration*)decl)->SetUsed();
                }
            }
        }
        break;
    case ANT_ARRAY_TYPE:
        {
            AstArrayType *node = (AstArrayType*)type_spec;
            if (!CheckArrayIndicesInTypes(node, mode)) {
                success = false;
            }
            if (!CheckTypeSpecification(node->element_type_, mode == TSCM_INITEDVAR ? TSCM_INITEDVAR : TSCM_STD)) {
                success = false;
            }
        }
        break;
    case ANT_MAP_TYPE:
        {
            AstMapType *node = (AstMapType*)type_spec;
            if (!CheckTypeSpecification(node->key_type_, TSCM_STD)) {
                success = false;
            } else if (!node->key_type_->SupportsEqualOperator()) {
                Error("Key type must support the == operator (not be a class/interface/map)", node->key_type_);
                success = false;
            }
            if (!CheckTypeSpecification(node->returned_type_, TSCM_STD)) {
                success = false;
            }
        }
        break;
    case ANT_POINTER_TYPE:
        {
            AstPointerType *node = (AstPointerType*)type_spec;
            if (!CheckTypeSpecification(node->pointed_type_, TSCM_REFERENCED)) {
                success = false;
            }
        }
        break;
    case ANT_FUNC_TYPE:
        {
            AstFuncType *node = (AstFuncType*)type_spec;
            int ii;

            for (ii = 0; ii < (int)node->arguments_.size(); ++ii) {
                VarDeclaration *arg = node->arguments_[ii];
                if (arg->type_spec_ == nullptr) {
                    Error("Unspecified type", arg);
                    success = false;
                } else if (!CheckTypeSpecification(arg->type_spec_, TSCM_REFERENCED)) {
                    success = false;
                } else {
                    if (arg->initer_ != nullptr) {
                        if (!arg->HasOneOfFlags(VF_READONLY)) {
                            Error("You can apply argument initers only to input parameters", arg);
                            success = false;
                        } else if (!IsArgTypeEligibleForAnIniter(arg->type_spec_)) {
                            Error("You can only init arguments of type: numeric, enum, string, bool, pointer (the latter only with null)", arg);
                            success = false;
                        } else if (arg->initer_->GetType() == ANT_INITER) {
                            Error("Only a single value is allowed here(not an aggregate initializer)", arg);
                            success = false;
                        } else if (!CheckIniter(arg->type_spec_, arg->initer_)) {
                            success = false;
                        } else if (!IsCompileTimeConstant((IAstExpNode*)arg->initer_)) {
                            Error("The argument's initer must be a compile time constant !", arg);
                            success = false;
                        }
                    }
                    if (arg->name_ != "") {
                        for (int jj = 0; jj < ii; ++jj) {
                            VarDeclaration *other = node->arguments_[jj];
                            if (other != nullptr && other->name_ == arg->name_) {
                                Error("Duplicated argument name", arg);
                                success = false;
                                break;
                            }
                        }
                    }
                }
            }
            bool is_void = node->return_type_->GetType() == ANT_BASE_TYPE && ((AstBaseType*)type_spec)->base_type_ == TOKEN_VOID;
            if (!is_void) {
                if (!CheckTypeSpecification(node->return_type_, TSCM_RETVALUE)) {
                    success = false;
                } else if (!IsArgTypeEligibleForAnIniter(node->return_type_)) {
                    Error("You can only return values of type: numeric, enum, string, bool, pointer", node->return_type_);
                }
            }
        }
        break;
    case ANT_ENUM_TYPE:
        CheckEnum((AstEnumType*)type_spec);
        break;
    case ANT_INTERFACE_TYPE:
        CheckInterface((AstInterfaceType*)type_spec);
        break;
    case ANT_CLASS_TYPE:
        in_class_declaration_ = true;
        CheckClass((AstClassType*)type_spec);
        in_class_declaration_ = false;
        break;
    default:
        success = false;
        break;
    }
    return(success);
}

bool AstChecker::CheckIniter(IAstTypeNode *type_spec, IAstNode *initer)
{
    type_spec = SolveTypedefs(type_spec);
    if (type_spec == nullptr || initer == nullptr) return(true);  // silent
    switch (type_spec->GetType()) {
    case ANT_BASE_TYPE:
    case ANT_POINTER_TYPE:
    case ANT_FUNC_TYPE:
    case ANT_CLASS_TYPE:
    case ANT_INTERFACE_TYPE:
    case ANT_ENUM_TYPE:
        if (initer->GetType() == ANT_INITER) {
            Error("Only a single value is allowed here (not an aggregate initializer)", initer);
            return(false);
        } else {
            ExpressionAttributes dst_attr, src_attr;

            dst_attr.InitWithTree(type_spec, nullptr, true, true, this);
            CheckExpression((IAstExpNode*)initer, &src_attr, ExpressionUsage::READ);
            if (dst_attr.IsOnError() || src_attr.IsOnError()) {
                return(false);
            }
            return(CanAssign(&dst_attr, &src_attr, initer));
        }
        break;
    case ANT_ARRAY_TYPE:
        if (initer->GetType() != ANT_INITER) {
            Error("You must init an aggregate with an aggregate initializer !", initer);
            return(false);
        } else if (!CheckArrayIniter((AstArrayType*)type_spec, initer)) {
            return(false);
        }
        break;
    case ANT_MAP_TYPE:
        if (initer->GetType() != ANT_INITER) {
            Error("You must init a map with an aggregate initializer !", initer);
            return(false);
        } else {
            char errorstr[200];
            AstIniter *node = (AstIniter*)initer;
            AstMapType *map = (AstMapType*)type_spec;
            int array_size = node->elements_.size();
            int ii = 0;
            while (ii < array_size - 1) {
                if (!CheckIniter(map->key_type_, node->elements_[ii])) {
                    sprintf(errorstr, "The initializer #%d is not of the type of the map key", ii + 1);
                    Error(errorstr, initer);
                    return(false);
                }
                if (!CheckIniter(map->returned_type_, node->elements_[ii+1])) {
                    sprintf(errorstr, "The initializer #%d is not of the type of the map element", ii + 2);
                    Error(errorstr, initer);
                    return(false);
                }
                ii += 2;
            }
            if (ii < array_size) {
                Error("The initializer of a map must have an even number of items (alternating keys and values)", initer);
                return(false);
            }
        }
        break;
    default:
        Error("You can't init this kind of variable", initer);
        return(false);
    }
    return(true);
}

bool AstChecker::CheckArrayIniter(AstArrayType *type_spec, IAstNode *initer)
{
    bool success = true;

    if (initer->GetType() != ANT_INITER) {
        Error("You must init an aggregate with an aggregate initializer !", initer);
        return(false);
    }
    AstIniter *node = (AstIniter*)initer;
    size_t array_size = type_spec->dimension_;
    size_t initer_size = node->elements_.size();
    size_t to_check = initer_size;                 // in case of error we may want to check a subset

    if (!type_spec->is_dynamic_) {

        // if the array is static and its size is determined by the initializer, set the size (es: []int = {...};) 
        // taking the max of all passes is good for the lower indices multi-dimensional arrays
        if (type_spec->expression_ == nullptr && initer_size > array_size) {
            array_size = initer_size;
            type_spec->dimension_ = array_size;
            type_spec->dimension_was_computed_ = true;
        }

        // too many initers ?
        if (array_size < initer_size) {
            to_check = array_size;
            Error("Too many initializers", initer);
            success = false;
        }
    }

    IAstTypeNode *etype = SolveTypedefs(type_spec->element_type_);  // makes the loop faster if there is a typedef to solve
    for (size_t ii = 0; ii < to_check; ++ii) {
        if (!CheckIniter(etype, node->elements_[ii])) {
            success = false;
            break;
        }
    }
    return(success);
}

void AstChecker::CheckEnum(AstEnumType *declaration)
{
    int64_t cur_index = 0;
    int num_elements = (int)declaration->items_.size();

    declaration->indices_.reserve(num_elements);
    for (int ii = 0; ii < num_elements; ++ii) {
        const string *name = &declaration->items_[ii];
        for (int jj = 0; jj < ii; ++jj) {
            if ((*name) == declaration->items_[jj]) {
                string error = "Enum case \"";
                error += name;
                error += "\" is duplicated";
                Error(error.c_str(), declaration);
            }
        }
        IAstExpNode *initer = declaration->initers_[ii];
        int64_t initer_value = 0;
        bool initer_valid = false;
        if (initer != nullptr) {
            ExpressionAttributes attr;
            CheckExpression(initer, &attr, ExpressionUsage::READ);
            if (!attr.IsOnError()) {
                if (!VerifyIndexConstness(initer) || !attr.HasKnownValue() || !attr.IsInteger() || !attr.FitsSigned(32)) {
                    Error("The enum case initer must be a compile time integer constant and fit an i32 type !", initer);
                } else {
                    attr.GetSignedIntegerValue(&initer_value);
                    if (initer_value < cur_index && ii != 0) {                        
                        Error("The enum case value must be greter than the previous one.", initer);                      
                    } else {
                        initer_valid = true;
                        declaration->indices_.push_back((int32_t)initer_value);
                        cur_index = initer_value + 1;
                    }
                }
            }
        }
        if (!initer_valid) {
            declaration->indices_.push_back((int32_t)cur_index++);
        }
    }
    if (cur_index >= 0x80000000) {
        Error("The enum case value overflows i32 capacity.", declaration);                      
    }
}

void AstChecker::CheckInterface(AstInterfaceType *declaration)
{
    vector<AstNamedType*>   origins;

    int func_count = (int)declaration->members_.size();
    declaration->first_hinherited_member_ = func_count;
    for (int ii = 0; ii < func_count; ++ii) {
        FuncDeclaration *fdecl = declaration->members_[ii];
        CheckMemberFuncDeclaration(fdecl, true);
        for (int jj = 0; jj < ii; ++jj) {
            if (declaration->members_[jj]->name_ == fdecl->name_) {
                Error("Duplicated member name", declaration->members_[ii]);
                break;
            }
        }
    }

    // conflict between interfaces' declarations
    int anc_count = (int)declaration->ancestors_.size();
    for (int ii = 0; ii < anc_count; ++ii) {
        AstNamedType *typespec = declaration->ancestors_[ii];

        bool duplicated = false;
        for (int jj = 0; jj < ii; ++jj) {
            if (declaration->ancestors_[jj]->IsCompatible(typespec, FOR_EQUALITY)) {
                Error("Interface included twice", typespec);
                duplicated = true;
                break;
            }
        }
        if (!duplicated) {
            CheckNameConflictsInIfFunctions(typespec, nullptr, &declaration->members_, &origins, func_count);
        }
    }
}

void AstChecker::CheckClass(AstClassType *declaration)
{
    vector<AstNamedType*>   origins;

    // check member variables 
    int num_vars = (int)declaration->member_vars_.size();
    for (int ii = 0; ii < num_vars; ++ii) {
        VarDeclaration *decl = declaration->member_vars_[ii];
        CheckMemberVar(decl);

        // if member can't be copied the class can't be copied as well !!
        if (declaration->can_be_copied) {
            IAstTypeNode *solved = SolveTypedefs(decl->type_spec_);
            if (solved != nullptr && solved->GetType() == ANT_CLASS_TYPE) {
                if (!((AstClassType*)solved)->can_be_copied) {
                    declaration->DisableCopy();
                }
            }
        }

        for (int jj = 0; jj < ii; ++jj) {
            if (declaration->member_vars_[jj]->name_ == decl->name_) {
                Error("Duplicated member name", decl);
                break;
            }
        }
    }

    // check functions
    int func_count = (int)declaration->member_functions_.size();
    declaration->first_hinherited_member_ = func_count;
    for (int ii = 0; ii < func_count; ++ii) {
        FuncDeclaration *fdecl = declaration->member_functions_[ii];
        string *implementor = &declaration->fn_implementors_[ii];
        if (*implementor != "") {
            bool found = false;
            bool has_function = false;
            if (fdecl->is_muting_) {
                Error("Muting attribute can't be assigned to a delegating function (depends on the delegated object)", fdecl);
            }
            for (int jj = 0; jj < num_vars; ++jj) {
                if (declaration->member_vars_[jj]->name_ == *implementor) {
                    found = true;
                    declaration->member_vars_[jj]->SetUsageFlags(ExpressionUsage::BOTH);
                    IAstTypeNode *mt = declaration->member_vars_[jj]->weak_type_spec_;
                    mt = SolveTypedefs(mt);
                    if (mt != nullptr && mt->GetType() == ANT_CLASS_TYPE) {
                        AstClassType *impl_decl = (AstClassType*)mt;
                        for (int kk = 0; kk < (int)impl_decl->member_functions_.size(); ++kk) {
                            if (impl_decl->member_functions_[kk]->name_ == fdecl->name_) {
                                has_function = true;
                                FuncDeclaration *implementing_fun = impl_decl->member_functions_[kk];
                                fdecl->function_type_ = implementing_fun->function_type_;
                                fdecl->SetMuting(implementing_fun->is_muting_);
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            if (!found) {
                Error("Implementor not found", fdecl);
            } else if (!has_function) {
                Error("The delegated member has not the required function", fdecl);
            }
            declaration->implemented_.push_back(true);
        } else {
            CheckMemberFuncDeclaration(fdecl, false);
            declaration->implemented_.push_back(false);
        }
        bool duplicated = false;
        for (int jj = 0; jj < ii; ++jj) {
            if (declaration->member_functions_[jj]->name_ == fdecl->name_) {
                Error("Duplicated member name", fdecl);
                duplicated = true;
                break;
            }
        }
        for (int jj = 0; jj < num_vars; ++jj) {
            if (declaration->member_vars_[jj]->name_ == fdecl->name_) {
                Error("Duplicated member name", fdecl);
                break;
            }
        }
    }

    // conflict between interfaces' declarations
    int anc_count = (int)declaration->member_interfaces_.size();
    for (int ii = 0; ii < anc_count; ++ii) {
        AstNamedType *typespec = declaration->member_interfaces_[ii];
        string *implementor = &declaration->if_implementors_[ii];
        bool has_implementor = *implementor != "";

        if (has_implementor) {
            bool found = false;
            bool has_interface = false;
            for (int jj = 0; jj < num_vars; ++jj) {
                if (declaration->member_vars_[jj]->name_ == *implementor) {
                    found = true;
                    declaration->member_vars_[jj]->SetUsageFlags(ExpressionUsage::BOTH);
                    IAstTypeNode *mt = declaration->member_vars_[jj]->weak_type_spec_;
                    mt = SolveTypedefs(mt);
                    if (mt != nullptr && mt->GetType() == ANT_CLASS_TYPE) {
                        AstClassType *impl_decl = (AstClassType*)mt;
                        for (int kk = 0; kk < (int)impl_decl->member_interfaces_.size(); ++kk) {
                            if (impl_decl->member_interfaces_[kk]->IsCompatible(typespec, FOR_EQUALITY)) {
                                has_interface = true;
                            }
                        }
                    }
                    break;
                }
            }
            if (!found) {
                Error("Implementor not found", typespec);
            } else if (!has_interface) {
                Error("The delegated member has not the required interface", typespec);
            }
        }

        bool duplicated = false;
        for (int jj = 0; jj < ii; ++jj) {
            if (declaration->member_interfaces_[jj]->IsCompatible(typespec, FOR_EQUALITY)) {
                Error("Interface included twice", typespec);
                duplicated = true;
                break;
            }
        }
        if (!duplicated) {
            CheckNameConflictsInIfFunctions(typespec, &declaration->member_vars_,
                &declaration->member_functions_, &origins, func_count);
            while (declaration->implemented_.size() < declaration->member_functions_.size()) {
                declaration->implemented_.push_back(has_implementor);
                declaration->fn_implementors_.push_back(*implementor);
            }
        }
    }
}

void AstChecker::CheckMemberFuncDeclaration(FuncDeclaration *declaration, bool from_interface_declaration)
{
    bool isok = true;
    AstFuncType *functype = declaration->function_type_;

    if (!CheckTypeSpecification(functype, TSCM_STD)) {
        isok = false;
    }
    if (stricmp(declaration->name_.c_str(), "finalize") == 0) {
        if (strcmp(declaration->name_.c_str(), "finalize") != 0) {
            Error("Did you mean 'finalize' (please check the case) ?", declaration);
            isok = false;
        } else {
            if (declaration->function_type_->arguments_.size() != 0 || !declaration->function_type_->ReturnsVoid()) {
                Error("The finalize function must have no arguments and no return value", declaration);
                isok = false;
            }
        }
        if (from_interface_declaration) {
            Error("No finalize function allowed in interfaces", declaration);
        } else if (!declaration->IsPublic()) {
            Error("Finalize function must be public", declaration);
        }
    }
    if (isok) {
        declaration->SetOk();
    }
}

// NOTE: Updates member_functions, origins
void AstChecker::CheckNameConflictsInIfFunctions(AstNamedType *typespec,
    vector<VarDeclaration*> *member_vars,
    vector<FuncDeclaration*> *member_functions,
    vector<AstNamedType*> *origins,
    int first_inherited_fun)
{
    string ancestor_name;

    // this is needed to set the wp_decl_ pointer. Tricky: TSCM_REFERENCED to allow interfaces.
    CheckTypeSpecification(typespec, TSCM_REFERENCED);

    // typespec must refer to an interface !! 
    if (typespec->wp_decl_ == nullptr) return;
    typespec->AppendFullName(&ancestor_name);
    IAstTypeNode *decl = SolveTypedefs(typespec->wp_decl_->type_spec_);
    if (decl != nullptr) {
        if (decl->GetType() == ANT_INTERFACE_TYPE) {
            AstInterfaceType *ancestor = (AstInterfaceType*)decl;
            for (int jj = 0; jj < (int)ancestor->members_.size(); ++jj) {
                FuncDeclaration *function = ancestor->members_[jj];
                bool duplicated = false;
                if (member_vars != nullptr) {
                    for (int kk = 0; kk < (int)member_vars->size() && !duplicated; ++kk) {
                        VarDeclaration *cmp_var = (*member_vars)[kk];
                        if (function->name_ == cmp_var->name_) {
                            string error = "Var member name conflicts with function from interface ";
                            error += ancestor_name;
                            Error(error.c_str(), cmp_var);
                            duplicated = true;
                        }
                    }
                }
                for (int kk = 0; kk < (int)member_functions->size() && !duplicated; ++kk) {
                    FuncDeclaration *cmp_function = (*member_functions)[kk];
                    if (function->name_ == cmp_function->name_) {
                        if (kk < first_inherited_fun) {
                            string error = "Function member conflicts with function from interface ";
                            error += ancestor_name;
                            Error(error.c_str(), cmp_function);
                            duplicated = true;
                        } else if (!function->function_type_->IsCompatible(cmp_function->function_type_, FOR_EQUALITY)) {
                            string error = "Function ";
                            error += function->name_;
                            error += " from interface ";
                            error += ancestor_name;
                            error += " conflicts (has same name, different type) with a function from interface ";
                            (*origins)[jj - first_inherited_fun]->AppendFullName(&error);
                            Error(error.c_str(), typespec);
                            duplicated = true;
                        }
                    }
                }
                if (!duplicated) {
                    member_functions->push_back(function);
                    origins->push_back(typespec);
                }
            }
        } else {
            Error("Name doesn't refer to an object of type interface", typespec);
        }
    }
}

void AstChecker::CheckBlock(AstBlock *block, bool open_scope)
{
    string                  text;
    int                     ii;

    if (open_scope) {
        symbols_->OpenScope();
    }
    for (ii = 0; ii < (int)block->block_items_.size(); ++ii) {
        IAstNode *node = block->block_items_[ii];
        AstNodeType nodetype = CheckStatement(node);
        if ((nodetype == ANT_RETURN || nodetype == ANT_SIMPLE) && ii < (int)block->block_items_.size() - 1) {
            Error("Dead code after terminating statement", node);
        }
    }
    if (check_usage_errors_) {
        CheckInnerBlockVarUsage();
    }
    symbols_->CloseScope();
}

AstNodeType AstChecker::CheckStatement(IAstNode *node)
{
    AstNodeType nodetype = node->GetType();
    switch (nodetype) {
    case ANT_VAR:
        CheckVar((VarDeclaration*)node);
        ((VarDeclaration*)node)->SetFlags(VF_ISLOCAL);
        break;
    case ANT_UPDATE:
        CheckUpdateStatement((AstUpdate*)node);
        break;
    case ANT_INCDEC:
        CheckIncDec((AstIncDec*)node);
        break;
    case ANT_SWAP:
        CheckSwap((AstSwap*)node);
        break;
    case ANT_WHILE:
        CheckWhile((AstWhile*)node);
        break;
    case ANT_IF:
        CheckIf((AstIf*)node);
        break;
    case ANT_FOR:
        CheckFor((AstFor*)node);
        break;
    case ANT_SWITCH:
        CheckSwitch((AstSwitch*)node);
        break;
    case ANT_TYPESWITCH:
        CheckTypeSwitch((AstTypeSwitch*)node);
        break;
    case ANT_SIMPLE:
        CheckSimpleStatement((AstSimpleStatement*)node);
        break;
    case ANT_RETURN:
        CheckReturn((AstReturn*)node);
        break;
    case ANT_FUNCALL:
        {
            ExpressionAttributes    attr;
            CheckFunCall((AstFunCall*)node, &attr);
        }
        break;
    case ANT_BLOCK:
        CheckBlock((AstBlock*)node, true);
        break;
    }
    return(nodetype);
}

void AstChecker::CheckUpdateStatement(AstUpdate *node)
{
    ExpressionAttributes attr_left, attr_right, attr_src;
    string  text;

    CheckExpression(node->left_term_, &attr_left, node->operation_ == TOKEN_ASSIGN ? ExpressionUsage::WRITE : ExpressionUsage::BOTH);
    CheckExpression(node->right_term_, &attr_right, ExpressionUsage::READ);
    if (!attr_left.IsOnError() && !attr_right.IsOnError()) {
        IAstTypeNode *type_node = attr_left.GetTypeTree();
        if (type_node != nullptr && !NodeIsConcrete(type_node)) {
            Error("Left operand has not a concrete type", node);
        } else if (node->operation_ == TOKEN_ASSIGN) {
            CanAssign(&attr_left, &attr_right, node);
        } else if (attr_left.RequiresPromotion()) {
            Error("Can't use update operators on 8/16 bit integers (a cast and assignment are required)", node);
        } else {
            Token operation;

            switch (node->operation_) {
            case TOKEN_UPD_PLUS:    operation = TOKEN_PLUS;     break;
            case TOKEN_UPD_MINUS:   operation = TOKEN_MINUS;    break;
            case TOKEN_UPD_MPY:     operation = TOKEN_MPY;      break;
            case TOKEN_UPD_DIVIDE:  operation = TOKEN_DIVIDE;   break;
            case TOKEN_UPD_XOR:     operation = TOKEN_XOR;      break;
            case TOKEN_UPD_MOD:     operation = TOKEN_MOD;      break;
            case TOKEN_UPD_SHR:     operation = TOKEN_SHR;      break;
            case TOKEN_UPD_SHL:     operation = TOKEN_SHL;      break;
            case TOKEN_UPD_AND:     operation = TOKEN_AND;      break;
            case TOKEN_UPD_OR:      operation = TOKEN_OR;       break;
            }
            attr_src = attr_left;
            if (!attr_src.UpdateWithBinopOperation(&attr_right, operation, &text)) {
                Error(text.c_str(), node);
            } else {
                CanAssign(&attr_left, &attr_right, node);
            }
        }
    }
}

void AstChecker::CheckIncDec(AstIncDec *node)
{
    ExpressionAttributes attr;

    CheckExpression(node->left_term_, &attr, ExpressionUsage::BOTH);
    if (!attr.IsOnError() && !attr.CanIncrement()) {
        Error("Increment and Decrement operations can be applied only to writable integer variables", node);
    }
}

void AstChecker::CheckSwap(AstSwap *node)
{
    ExpressionAttributes attr_left, attr_right;

    CheckExpression(node->left_term_, &attr_left, ExpressionUsage::BOTH);
    CheckExpression(node->right_term_, &attr_right, ExpressionUsage::BOTH);
    if (!attr_left.IsOnError() && !attr_right.IsOnError() && !attr_left.CanSwap(&attr_right, this)) {
        Error("You can only swap two writable variables of IDENTICAL type", node);
    }
}

void AstChecker::CheckWhile(AstWhile *node)
{
    ExpressionAttributes attr;

    CheckExpression(node->expression_, &attr, ExpressionUsage::READ);
    if (!attr.IsOnError() && !attr.IsBool()) {
        Error("Expression must evaluate to a bool", node);
    }
    ++loops_nesting;
    CheckBlock(node->block_, true);
    --loops_nesting;
}

void AstChecker::CheckIf(AstIf *node)
{
    int     ii;

    for (ii = 0; ii < (int)node->expressions_.size(); ++ii) {
        ExpressionAttributes attr;
        CheckExpression(node->expressions_[ii], &attr, ExpressionUsage::READ);
        if (!attr.IsOnError() && !attr.IsBool()) {
            Error("Expression must evaluate to a bool", node->expressions_[ii]);
        }
        CheckBlock(node->blocks_[ii], true);
    }
    if (node->default_block_ != nullptr) {
        CheckBlock(node->default_block_, true);
    }
}

//
// For index and iterators must be new symbols or index and iterator of a previous loop !!
//
void AstChecker::CheckFor(AstFor *node)
{
    VarDeclaration  *index_declaration = nullptr;
    VarDeclaration  *iterator_declaration = nullptr;
    VarDeclaration  *iterated_declaration = nullptr;

    // check the index
    if (node->index_ != nullptr) {
        const char *index_name = node->index_->name_.c_str();
        IAstDeclarationNode *index = SearchDeclaration(index_name, node->index_);
        if (index != nullptr) {
            if (IsGoodForIndex(index)) {
                node->index_referenced_ = true;
                index_declaration = (VarDeclaration*)index;
            } else {
                Error("The index name must be either a new name or the name of a reusable index", node->index_);
            }
        } else {
            assert(node->index_->type_spec_ == nullptr);
            node->index_->SetType(new AstBaseType(TOKEN_UINT64));
            symbols_->InsertName(index_name, node->index_);         // directly call symbols_ to avoid duplicate error messages
            index_declaration = node->index_;
        }
    }

    // using an existing iterator ?
    assert(node->iterator_ != nullptr);
    const char *iterator_name = node->iterator_->name_.c_str();
    IAstDeclarationNode *iterator = SearchDeclaration(iterator_name, node->iterator_);

    // SINCE I've decided the iterator is in the block scope
    if (iterator != nullptr) {
        Error("The iterator name must be a new name", node->iterator_);
    }
    symbols_->OpenScope();

    // foreach or integer range for ?
    if (node->set_ != nullptr) {
        ExpressionAttributes    attr;

        CheckExpression(node->set_, &attr, ExpressionUsage::READ);
        IAstTypeNode *iterator_type = attr.GetIteratorType(); // return nullptr if not an array or map.
        iterated_declaration = GetIteratedVar(node->set_);
        if (iterator_type == nullptr) {
            if (attr.IsString()) {

                // CASE 1: rune * iterating in a string
                assert(node->iterator_->type_spec_ == nullptr);
                node->iterator_->SetType(new AstBaseType(TOKEN_UINT32));
                symbols_->InsertName(iterator_name, node->iterator_);
                iterator_declaration = node->iterator_;
                iterator_declaration->SetTheIteratedVar(iterated_declaration);
            } else {
                Error("The variable you are iterating in must be an array, map or string", node->set_);
            }
        } else {

            // CASE 2: anytype* iterating in a vector or map
            node->iterator_->weak_type_spec_ = iterator_type;
            symbols_->InsertName(iterator_name, node->iterator_);
            iterator_declaration = node->iterator_;
            iterator_declaration->SetTheIteratedVar(iterated_declaration);
        }
    } else {
        ExpressionAttributes    low, high, step;
        Token                   ittype = TOKEN_INT32;
        bool                    onerror = false;

        // CASE 3: int32/int64 integer loop (iterator is not a reference)
        CheckExpression(node->low_, &low, ExpressionUsage::READ);
        CheckExpression(node->high_, &high, ExpressionUsage::READ);
        if (node->step_ != nullptr) {
            CheckExpression(node->step_, &step, ExpressionUsage::READ);
        }
        if (!ExpressionAttributes::GetIntegerIteratorType(&ittype, &low, &high, &step)) {
            Error("For loop extrema and step must fit int64", node);
            onerror = true;
        }
        if (node->step_ != nullptr) {
            node->step_value_ = 0;
            if (step.GetSignedIntegerValue(&node->step_value_) && node->step_value_ == 0) {
                Error("Step can't be 0", node);
                onerror = true;
            }
        } else {
            int64_t start, stop;

            node->step_value_ = 1;
            if (low.GetSignedIntegerValue(&start) && high.GetSignedIntegerValue(&stop) && start > stop) {
                node->step_value_ = -1;
            }
        }
        assert(node->iterator_->type_spec_ == nullptr);
        node->iterator_->SetType(new AstBaseType(ittype));
        symbols_->InsertName(iterator_name, node->iterator_);
        iterator_declaration = node->iterator_;
    }

    // be sure the iterator and index are not reused inside the block
    if (index_declaration != nullptr) {
        index_declaration->SetFlags(VF_ISBUSY);
    }
    if (iterated_declaration != nullptr) {
        iterated_declaration->SetFlags(VF_IS_ITERATED);
    }

    ++loops_nesting;
    CheckBlock(node->block_, false);
    --loops_nesting;

    if (index_declaration != nullptr) {
        index_declaration->ClearFlags(VF_ISBUSY);
    }
    if (iterated_declaration != nullptr) {
        iterated_declaration->ClearFlags(VF_IS_ITERATED);
    }
}

void AstChecker::CheckSwitch(AstSwitch *node)
{
    ExpressionAttributes attr;
    bool switch_error = false;
    bool cases_errors = false;    
    AstEnumType *enum_type = nullptr;
    vector<int64_t> case_values;
    bool is_integer = false; 
    string error;

    CheckExpression(node->switch_value_, &attr, ExpressionUsage::READ);
    if (!attr.IsOnError()) {
        if (attr.IsEnum()) {
            enum_type = (AstEnumType*)attr.GetTypeTree();
        } else if (attr.IsInteger()) {
            is_integer = true;
            if (attr.RequiresPromotion()) {
                attr.InitWithInt32(0);  // just done to force the type to int32.
            }
        } else {
            Error("Switch expression must be of integer or enum type", node->switch_value_);
            switch_error = true;
        }
    } else {
        switch_error = true;
    }
    if (node->statement_top_case_.size() == 0 || node->statement_top_case_.size() == 1 && node->has_default) {
        Error("The Switch statement must have at least a non-default case", node);       
    }
    for (int ii = 0; ii < (int)node->case_values_.size(); ++ii) {
        IAstExpNode *exp = node->case_values_[ii];
        if (exp != nullptr) {
            ExpressionAttributes cmp_attr;
            CheckExpression(exp, &cmp_attr, ExpressionUsage::READ);
            if (!switch_error && !cmp_attr.IsOnError()) {
                ExpressionAttributes tmp = cmp_attr;
                if (!tmp.UpdateWithRelationalOperation(this, &attr, TOKEN_EQUAL, &error)) {
                    Error("Switch cases must match the switch expression type (or be scalar values)", exp);
                    cases_errors = true;
                } else if (!VerifyIndexConstness(exp) || is_integer &&
                    !cmp_attr.IsCaseValueCompatibleWithSwitchExpression(&attr)) {
                    Error("Switch case must be a simple integer compile time constant and fit the range of the switch type. Can't contain the ^, ~ operators.", exp);
                    cases_errors = true;
                } else {
                    int64_t value = 0;
                    cmp_attr.GetSignedIntegerValue(&value);
                    case_values.push_back(value);
                }
            }
        }
    }

    if (!cases_errors && !switch_error) {
        case_values.sort_shallow_copy(0, case_values.size());
        bool is_ok = true;
        for (int ii = 1; is_ok && ii < case_values.size(); ++ii) {
            if (case_values[ii] == case_values[ii - 1]) {
                Error("Duplicated case values !", node);
                is_ok = false;
            }
        }
        if (enum_type != nullptr && !node->has_default && is_ok) {

            // we require all enum values to be present
            bool is_ok = case_values.size() == enum_type->indices_.size();
            for (int ii = 0; is_ok && ii < case_values.size(); ++ii) {
                if (case_values[ii] != enum_type->indices_[ii]) {
                    is_ok = false;
                }
            }

            if (!is_ok) {
                Error("Not All Enum Values are present", node);
            }
        }
    }

    for (int ii = 0; ii < (int)node->statements_.size(); ++ii) {
        IAstNode *statement = node->statements_[ii];
        if (statement != nullptr) {
            CheckStatement(statement);
        }
    }
}

void AstChecker::CheckTypeSwitch(AstTypeSwitch *node)
{
    const char *reference_name = node->reference_->name_.c_str();
    IAstDeclarationNode *reference = SearchDeclaration(reference_name, node->reference_);
    if (reference != nullptr) {
        Error("The reference name must be a new name", node->reference_);
    }

    ExpressionAttributes    attr;
    CheckExpression(node->expression_, &attr, ExpressionUsage::READ);
    if (node->expression_->HasFunction()) {
        Error("The typeswitch expression can't include function calls", node->expression_);
    }
    IAstTypeNode *referenced_type = attr.GetTypeTree();
    IAstTypeNode *pointed_type = SolveTypedefs(attr.GetPointedType());
    bool is_interface = referenced_type != nullptr && referenced_type->GetType() == ANT_INTERFACE_TYPE;
    bool is_interface_ptr = pointed_type != nullptr && pointed_type->GetType() == ANT_INTERFACE_TYPE;
    if (!is_interface && !is_interface_ptr) {
        Error("The expression must evaluate to an interface or interface pointer type", node->expression_);
        referenced_type = nullptr;
    }
    if (is_interface_ptr) {
        node->reference_->ClearFlags(VF_IS_REFERENCE);  // is the actual pointer, not a reference to pointer !
        node->SetSwitchOnInterfacePointer();
    }
    for (int ii = 0; ii < (int)node->case_types_.size(); ++ii) {
        IAstTypeNode *case_type = node->case_types_[ii];
        if (case_type != nullptr) {
            CheckTypeSpecification(case_type, TSCM_STD);
            IAstTypeNode *solved_type = SolveTypedefs(case_type);
            if (solved_type != nullptr && referenced_type != nullptr) {
                if (!AreInterfaceAndClass(referenced_type, solved_type, FOR_ASSIGNMENT)) {
                    Error("The case label type must be a class/interface implementing the typeswitch interface", case_type);
                } else if (solved_type->GetType() == ANT_POINTER_TYPE) {
                    if (!((AstPointerType*)solved_type)->CheckConstness(referenced_type, FOR_ASSIGNMENT)) {
                        Error("The case label type should be a const pointer", case_type);
                    }
                }
            }
        }
        IAstNode *statement = node->case_statements_[ii];
        if (statement != nullptr && case_type != nullptr) {
            symbols_->OpenScope();

            IAstTypeNode *savetype = node->reference_->type_spec_;

            // want to know if it is used
            node->reference_->ClearFlags(VF_WASREAD | VF_WASWRITTEN);
            node->reference_->weak_type_spec_ = case_type;
            node->reference_->type_spec_ = case_type;

            symbols_->InsertName(reference_name, node->reference_);
            CheckStatement(statement);

            node->reference_->type_spec_ = savetype;
            node->reference_->weak_type_spec_ = savetype;

            node->SetReferenceUsage(node->reference_->HasOneOfFlags(VF_WASREAD | VF_WASWRITTEN));
            symbols_->CloseScope();
        } else if (statement != nullptr) {
            CheckStatement(statement);
            node->SetReferenceUsage(false);
        } else {
            node->SetReferenceUsage(false);
        }
    }
}

void AstChecker::CheckSimpleStatement(AstSimpleStatement *node)
{
    if (loops_nesting == 0) {
        Error("You can only break/continue a for or while loop.", node);
    }
}

void AstChecker::CheckReturn(AstReturn *node)
{
    if (node->retvalue_ == nullptr) {
        if (!return_fake_variable_.IsOnError() && !return_fake_variable_.IsVoid()) {
            Error("You must return a value from a non-void function.", node);
        }
    } else {
        if (!return_fake_variable_.IsOnError() && return_fake_variable_.IsVoid()) {
            Error("You can't return a value from a void function.", node);
        } else {
            ExpressionAttributes    attr;

            CheckExpression(node->retvalue_, &attr, ExpressionUsage::READ);
            CanAssign(&return_fake_variable_, &attr, node);     // don't need to check - it emits the necessary messages.
        }
    }
}

void AstChecker::CheckExpression(IAstExpNode *node, ExpressionAttributes *attr, ExpressionUsage usage)
{
    switch (node->GetType()) {
    case ANT_INDEXING:
        CheckIndices((AstIndexing*)node, attr, usage);
        break;
    case ANT_FUNCALL:
        CheckFunCall((AstFunCall*)node, attr);
        break;
    case ANT_BINOP:
        if (((AstBinop*)node)->subtype_ == TOKEN_DOT) {
            CheckDotOp((AstBinop*)node, attr, usage, false);
        } else {
            CheckBinop((AstBinop*)node, attr);
        }
        break;
    case ANT_UNOP:
        CheckUnop((AstUnop*)node, attr);
        break;
    case ANT_EXP_LEAF:
        CheckLeaf((AstExpressionLeaf*)node, attr, usage);
        break;
    }
}

void AstChecker::CheckIndices(AstIndexing *node, ExpressionAttributes *attr, ExpressionUsage usage)
{
    bool                    failure;
    string                  error;
    ExpressionAttributes    low_attr, high_attr;

    CheckExpression(node->indexed_term_, attr, usage);
    failure = attr->IsOnError();
    if (node->lower_value_ != nullptr) {
        CheckExpression(node->lower_value_, &low_attr, ExpressionUsage::READ);
        failure = failure || low_attr.IsOnError();
    } else {
        Error("Missing index", node);
    }
    if (node->upper_value_ != nullptr) {
        Error("Currently ranges are not supported", node->upper_value_);
        CheckExpression(node->upper_value_, &high_attr, ExpressionUsage::READ);
        failure = failure || high_attr.IsOnError();
        failure = true;
    }
    if (!failure) {
        if (!attr->UpdateWithIndexing(&low_attr, &high_attr, node, &node->map_type_, this, &error)) {
            Error(error.c_str(), node);
        }
    } else {
        attr->SetError();
    }
    node->attr_ = *attr;
}

void AstChecker::CheckFunCall(AstFunCall *node, ExpressionAttributes *attr)
{
    bool                            failure;
    vector<ExpressionAttributes>    attributes;
    string                          error;
    AstFuncType                     *ftype = nullptr;

    CheckExpression(node->left_term_, attr, ExpressionUsage::READ);
    failure = attr->IsOnError();
    if (!failure) {
        ftype = attr->GetFunCallType();
    }
    if (node->arguments_.size() > 0) {
        attributes.reserve(node->arguments_.size());
        for (int ii = 0; ii < (int)node->arguments_.size(); ++ii) {
            AstArgument *arg = node->arguments_[ii];
            if (arg != nullptr) {
                ExpressionUsage usage = ExpressionUsage::READ;

                if (ftype != nullptr && ii < ftype->arguments_.size()) {
                    if (ftype->arguments_[ii]->HasOneOfFlags(VF_READONLY)) {
                        usage = ExpressionUsage::READ;
                    } else if (ftype->arguments_[ii]->HasOneOfFlags(VF_WRITEONLY)) {
                        usage = ExpressionUsage::WRITE;
                    } else {
                        usage = ExpressionUsage::BOTH;
                    }
                }
                CheckExpression(arg->expression_, &attributes[ii], usage);
                failure = failure || attributes[ii].IsOnError();
            }
        }
    }
    if (!failure) {
        if (!attr->UpdateWithFunCall(&attributes, &node->arguments_, &node->func_type_, this, &error)) {
            Error(error.c_str(), node);
        } else {
            CheckIfFunCallIsLegal(node->func_type_, node);
        }
    } else {
        attr->SetError();
    }
    node->attr_ = *attr;
}

void AstChecker::CheckBinop(AstBinop *node, ExpressionAttributes *attr)
{
    ExpressionAttributes attr_right;
    string               error;

    CheckExpression(node->operand_left_, attr, ExpressionUsage::READ);
    CheckExpression(node->operand_right_, &attr_right, ExpressionUsage::READ);
    switch (node->subtype_) {
    case TOKEN_POWER:
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_MOD:
    case TOKEN_SHR:
    case TOKEN_SHL:
    case TOKEN_AND:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_OR:
    case TOKEN_XOR:
    case TOKEN_MIN:
    case TOKEN_MAX:
        if (!attr->UpdateWithBinopOperation(&attr_right, node->subtype_, &error)) {
            Error(error.c_str(), node);
        }
        break;
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_GTE:
    case TOKEN_LTE:
    case TOKEN_DIFFERENT:
    case TOKEN_EQUAL:
        if (!attr->UpdateWithRelationalOperation(this, &attr_right, node->subtype_, &error)) {
            Error(error.c_str(), node);
        }
        break;
    case TOKEN_LOGICAL_AND:
    case TOKEN_LOGICAL_OR:
        if (!attr->UpdateWithBoolOperation(&attr_right, &error)) {
            Error(error.c_str(), node);
        }
        break;
    default:
        assert(false);
        break;
    }
    node->attr_ = *attr;
}

void AstChecker::CheckUnop(AstUnop *node, ExpressionAttributes *attr)
{
    if (node->subtype_ == TOKEN_AND) {
        CheckExpression(node->operand_, attr, ExpressionUsage::NONE);
        if (!attr->IsOnError()) {
            if (!FlagLocalVariableAsPointed(node->operand_) || !attr->TakeAddress()) {
                Error("You can apply the & unary operator only to a local variable.", node);
                attr->SetError();
            }
        }
    } else if (node->subtype_ == TOKEN_SIZEOF) {
        IAstTypeNode *type = node->type_;
        if (type == nullptr) {
            CheckExpression(node->operand_, attr, ExpressionUsage::READ);
            if (!attr->IsOnError()) {
                type = attr->GetAutoType();
                if (type == nullptr) {
                    Error("Sizeof argument must be a type, a variable or a portion of a variable", node);
                    attr->SetError();
                }
            }
        } else if (!CheckTypeSpecification(node->type_, TSCM_STD)) {
            attr->SetError();
            type = nullptr;
        }
        if (type != nullptr) {
            // sizeof(type);
            int size = type->SizeOf();
            if (size == 0) {
                Error("Sizeof doesn't apply to this type (it has variable size)", node);
                attr->SetError();
            } else {
                attr->InitWithInt32(size);
            }
        }
    } else {
        CheckExpression(node->operand_, attr, ExpressionUsage::READ);
        if (!attr->IsOnError()) {
            string error;
            if (!attr->UpdateWithUnaryOperation(node->subtype_, this, &error)) {
                Error(error.c_str(), node);
            }
        }
    }
    node->attr_ = *attr;
}

// 
// DotOp usages:
//
// <file tag>.<extern_symbol>
// <enum type>.<enum case>
// <class/interface instance>.<member function/variable>
// this.<member function/variable>
// value.builtin_function()
//
void AstChecker::CheckDotOp(AstBinop *node, ExpressionAttributes *attr, ExpressionUsage usage, bool dotop_left)
{
    // if left branch is a leaf or another dotop, call it directly with special options:
    // it could be a <file reference> or a <file reference>.<enum/class/if type>, which is disallowed in other cases.
    int pkg_index = -1;
    if (node->operand_left_->GetType() == ANT_EXP_LEAF) {
        AstExpressionLeaf* left_leaf = (AstExpressionLeaf*)node->operand_left_;
        CheckDotOpLeftLeaf(left_leaf, attr, ExpressionUsage::READ);
        pkg_index = left_leaf->pkg_index_;
    } else if (node->operand_left_->GetType() == ANT_BINOP && ((AstBinop*)node->operand_left_)->subtype_ == TOKEN_DOT) {
        CheckDotOp((AstBinop*)node->operand_left_, attr, ExpressionUsage::READ, true);
    } else {
        CheckExpression(node->operand_left_, attr, ExpressionUsage::READ);
    }

    // check the right branch
    AstExpressionLeaf* right_leaf = nullptr;
    if (node->operand_right_->GetType() == ANT_EXP_LEAF) {
        right_leaf = (AstExpressionLeaf*)node->operand_right_;
        if (right_leaf->subtype_ != TOKEN_NAME) {
            right_leaf = nullptr;
        }
    }
    if (right_leaf == nullptr) {
        Error("The term at the right of a dot operator must be a name", node);
        attr->SetError();
    } else if (pkg_index >= 0) {

        // case 1: <file tag>.<extern_symbol>
        bool is_private;
        IAstDeclarationNode *decl = SearchExternDeclaration(pkg_index, right_leaf->value_.c_str(), &is_private);
        if (is_private) {
            Error("Cannot access private symbol", right_leaf);
            attr->SetError();
        } else if (decl == nullptr) {
             Error("Undefined symbol", right_leaf);
             attr->SetError();
        } else {
            CheckNamedLeaf(decl, right_leaf, attr, usage, dotop_left);
        }
        right_leaf->attr_ = *attr;
    } else if (!attr->IsOnError()) {
        bool is_valid = false;

        // is a field selector operator or the case selector of an enum literal.
        // NOTE: here attr is the attribute of the left subtree.
        IAstTypeNode *tnode = attr->GetTypeTree();
        if (tnode != nullptr) {
            AstNodeType nodetype = tnode->GetType();
            if (nodetype == ANT_ENUM_TYPE) {

                // case 2: <enum type>.<enum case>
                is_valid = true;
                AstEnumType *enumnode = (AstEnumType*)tnode;
                int entry;
                for (entry = 0; entry < (int)enumnode->items_.size(); ++entry) {
                    if (enumnode->items_[entry] == right_leaf->value_) {
                        break;
                    }
                }
                if (entry >= (int)enumnode->items_.size()) {
                    //string message = right_leaf->value_ + " is not a case of the enum";
                    //Error(message.c_str(), right_leaf);
                    Error("Not a case of the enumeration", right_leaf);
                    attr->SetError();
                } else if (entry < (int)enumnode->indices_.size()) {
                    attr->SetEnumValue(enumnode->indices_[entry]);
                }
            } else {

                // case 3: <class/interface instance>.<member function/variable>
                //          this.<member function/variable>
                if (nodetype == ANT_POINTER_TYPE) {
                    AstPointerType *ptrnode = (AstPointerType*)tnode;
                    attr->InitWithTree(ptrnode->pointed_type_, nullptr, true, !ptrnode->isconst_, this);
                    tnode = attr->GetTypeTree();
                }
                if (tnode != nullptr) {
                    nodetype = tnode->GetType();
                    if (nodetype == ANT_CLASS_TYPE) {
                        is_valid = true;
                        AstClassType *classnode = (AstClassType*)tnode;
                        CheckMemberAccess(right_leaf, &classnode->member_functions_, &classnode->member_vars_, attr, usage);
                    } else if (nodetype == ANT_INTERFACE_TYPE) {
                        is_valid = true;
                        AstInterfaceType *ifnode = (AstInterfaceType*)tnode;
                        CheckMemberAccess(right_leaf, &ifnode->members_, nullptr, attr, usage);
                    }
                }                
            }
        }
        if (!is_valid) {

            // case 4: value.builtin_function()
            const char *name = right_leaf->value_.c_str();
            const char *signature = GetFuncSignature(name, attr);
            node->builtin_ = GetFuncTypeFromSignature(signature, attr);
            if (node->builtin_ == nullptr) {
                //Error("Before the dot operator you can place a file tag, an enum type, an interface, 'this' or a class instance", node);
                Error("Left operand is not compatible with the dot operator or the selected function", node);
                attr->SetError();
            } else {
                node->SetSignature(signature);
                if (signature[0] == 'M') {
                    if (!attr->IsWritable()) {
                        Error("Can't call a muting member function on a constant variable/field", right_leaf);
                    } else {
                        attr->SetUsageFlags(ExpressionUsage::WRITE);
                    }
                } 
                attr->InitWithTree(node->builtin_, nullptr, false, false, this);
            }
        }
    }
    node->attr_ = *attr;
}

//
// IN
// accessed: the right leaf
// member_functs/vars from the class/interface declaration
// attr: of the instance
// usage
//
// OUT:
// attr of the member
// accessed->wp_decl_ (points to the function/variable declaration)
// function/variable declaration's usage flags
// Error messages get emitted
//
void AstChecker::CheckMemberAccess(AstExpressionLeaf *accessed, 
                                   vector<FuncDeclaration*> *member_functions, vector<VarDeclaration*> *member_vars, 
                                   ExpressionAttributes *attr, ExpressionUsage usage)
{
    bool has_private_access = attr->GetTypeTree() == current_class_;        // i.e. the function is a member of the accessed class.
    bool class_is_writable = attr->IsWritable();
    bool in_notmuting = has_private_access && current_function_ != nullptr && !current_function_->is_muting_;
    for (int fun_idx = 0; fun_idx < (int)member_functions->size(); ++fun_idx) {
        if ((*member_functions)[fun_idx]->name_ == accessed->value_) {
            FuncDeclaration *decl = (*member_functions)[fun_idx];
            if (has_private_access || decl->IsPublic()) {
                accessed->wp_decl_ = decl;
                attr->InitWithTree(decl->function_type_, nullptr, false, false, this);
                decl->SetUsed();
                CheckIfFunCallIsLegal(decl->function_type_, accessed);
                if (decl->is_muting_) {
                    if (in_notmuting) {
                        Error("Can't call a muting member function from a non-muting one", accessed);
                    } else if (!class_is_writable) {
                        Error("Can't call a muting member function on a read-only instance (input argument or constant)", accessed);
                    } 
                }
                accessed->SetUMA(symbols_->FindLocalDeclaration(accessed->value_.c_str()) == nullptr);
            } else {
                Error("Can't access private member", accessed);
                attr->SetError();
            }
            return;
        }
    }
    if (attr->IsAVariable() && member_vars != nullptr) {
        for (int var_idx = 0; var_idx < (int)member_vars->size(); ++var_idx) {
            if ((*member_vars)[var_idx]->name_ == accessed->value_) {
                VarDeclaration *decl = (*member_vars)[var_idx];
                if (has_private_access || decl->IsPublic()) {
                    accessed->wp_decl_ = decl;
                    attr->InitWithTree(decl->weak_type_spec_, decl, true, !decl->HasOneOfFlags(VF_READONLY) && attr->IsWritable() && !in_notmuting, this);
                    decl->SetUsageFlags(usage);
                    CheckIfVarReferenceIsLegal(decl, accessed);
                    accessed->SetUMA(symbols_->FindLocalDeclaration(accessed->value_.c_str()) == nullptr);
                } else {
                    Error("Can't access private member", accessed);
                    attr->SetError();
                }
                return;
            }
        }
    }
    Error("Member not found", accessed);
    attr->SetError();
}

// with respect to other leaves can additionally be a file reference or an enum type.
void AstChecker::CheckDotOpLeftLeaf(AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage)
{
    if (node->subtype_ == TOKEN_THIS) {
        if (current_class_ != nullptr && current_function_ != nullptr)
        {
            attr->InitWithTree(current_class_, nullptr, true, current_function_->is_muting_, this);
            this_was_accessed_ = true;
        } else {
            Error("'This' is allowed only in member functions", node);
        }
        node->attr_ = *attr;
    } else if (node->subtype_ == TOKEN_NAME) {
        IAstDeclarationNode *decl = SearchDeclaration(node->value_.c_str(), node);
        if (decl != nullptr) {
            CheckNamedLeaf(decl, node, attr, usage, true);
        } else {
            int pkg = SearchAndLoadPackage(node->value_.c_str(), node, "Undefined symbol");
            if (pkg != -1) {
                node->pkg_index_ = pkg;
            } else {
                attr->SetError();
            }
        }
        node->attr_ = *attr;
    } else {
        CheckLeaf(node, attr, usage);
    }
}

void AstChecker::CheckLeaf(AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage)
{
    if (node->subtype_ == TOKEN_NAME) {
        IAstDeclarationNode *decl = SearchDeclaration(node->value_.c_str(), node);
        if (decl != nullptr) {
            CheckNamedLeaf(decl, node, attr, usage, false);
        } else {
            Error("Undefined symbol", node);
        }
    } else {
        string error;
        if (!attr->InitWithLiteral(node->subtype_, node->value_.c_str(), node->img_value_.c_str(),
            node->real_is_int_, node->real_is_negated_, node->img_is_negated_, &error)) {
            Error(error.c_str(), node);
        }
    }
    node->attr_ = *attr;
}

void AstChecker::CheckNamedLeaf(IAstDeclarationNode *decl, AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage, bool preceeds_dotop)
{
    if (decl->GetType() == ANT_VAR) {
        node->wp_decl_ = decl;
        VarDeclaration *var = (VarDeclaration*)decl;
        bool readonly = false;
        if (var->HasOneOfFlags(VF_IS_REFERENCE)) {
            if (var->weak_iterated_var_ != nullptr) {
                readonly = var->weak_iterated_var_->HasOneOfFlags(VF_READONLY);
            }
        } else {
            readonly = var->HasOneOfFlags(VF_READONLY);
        }
        attr->InitWithTree(var->weak_type_spec_, var, true, !readonly, this);
        if (var->HasOneOfFlags(VF_READONLY)) {
            if (var->initer_ != nullptr && var->initer_->GetType() != ANT_INITER) {
                attr->SetTheValueFrom(((IAstExpNode*)var->initer_)->GetAttr());
            }
        }
        var->SetUsageFlags(usage);
        CheckIfVarReferenceIsLegal(var, node);
    } else if (decl->GetType() == ANT_FUNC) {
        node->wp_decl_ = decl;
        FuncDeclaration *func = (FuncDeclaration*)decl;
        func->SetUsed();
        attr->InitWithTree(func->function_type_, nullptr, false, false, this);
    } else if (preceeds_dotop) {
        bool is_allowed_type = false;
        if (decl->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)decl;
            IAstTypeNode *tspec = SolveTypedefs(tdecl->type_spec_);
            if (tspec != nullptr) {
                if (tspec->GetType() == ANT_ENUM_TYPE/* || tspec->GetType() == ANT_CLASS_TYPE || tspec->GetType() == ANT_INTERFACE_TYPE*/) {
                    is_allowed_type = true;
                    node->wp_decl_ = decl;
                    attr->InitWithTree(tdecl->type_spec_, nullptr, false, false, this);
                    tdecl->SetUsed();
                }
            } else {
                is_allowed_type = true; // silent
            }
        }
        if (!is_allowed_type) {
            //Error("Before the dot operator you can place a file tag, an enum type, an interface, 'this' or a class instance", node);
            Error("Left operand is not compatible with the dot operator", node);
            attr->SetError();
        }
    } else {
        Error("Need a variable/const/function name here", node);
        attr->SetError();
    }
}

void AstChecker::SetUsageOnExpression(IAstExpNode *node, ExpressionUsage usage)
{
    switch (node->GetType()) {
    case ANT_INDEXING:
        SetUsageOnIndices((AstIndexing*)node, usage);
        break;
    case ANT_FUNCALL:    
        SetUsageOnFunCall((AstFunCall*)node, usage);
        break;
    case ANT_BINOP:
        if (((AstBinop*)node)->subtype_ == TOKEN_DOT) {
            SetUsageOnDotOp((AstBinop*)node, usage, false);
        } else {
            // this function is actually used for WRITE usage.
            // it doesn't apply to left and right expressions of a binop.
        }
        break;
    case ANT_UNOP:
        //CheckUnop((AstUnop*)node, attr);
        break;
    case ANT_EXP_LEAF:
        //CheckLeaf((AstExpressionLeaf*)node, attr, usage);
        break;
    }
}

void AstChecker::SetUsageOnIndices(AstIndexing *node, ExpressionUsage usage)
{

}

void AstChecker::SetUsageOnFunCall(AstFunCall *node, ExpressionUsage usage)
{

}

void AstChecker::SetUsageOnDotOp(AstBinop *node, ExpressionUsage usage, bool dotop_left)
{

}

void AstChecker::SetUsageOnLeaf(AstExpressionLeaf *node, ExpressionUsage usage, bool dotop_left)
{

}

void AstChecker::SetUsageOnNamedLeaf(IAstDeclarationNode *decl, AstExpressionLeaf *node, ExpressionAttributes *attr, ExpressionUsage usage, bool preceeds_dotop)
{

}

void AstChecker::InsertName(const char *name, IAstDeclarationNode *declaration)
{
    if (!symbols_->InsertName(name, declaration)) {
        string message = "Symbol '";
        message += name;
        message += "' is duplicated";
        Error(message.c_str(), declaration);
    }
}

IAstDeclarationNode *AstChecker::SearchDeclaration(const char *name, IAstNode *location)
{
    IAstDeclarationNode *node = symbols_->FindDeclaration(name);
    if (node == nullptr) return(nullptr);
    if (!node->IsPublic() && current_is_public_ && !in_function_block_) {
        Error("A public declaration cannot refer a private symbol", location);
    }
    return(node);
}

IAstDeclarationNode *AstChecker::ForwardSearchDeclaration(const char *name, IAstNode *location)
{
    // only allowed inside a class declaration
    //IAstDeclarationNode *referring_decl = root_->declarations_[current_];
    //if (referring_decl->GetType() != ANT_TYPE) return(nullptr);
    //IAstTypeNode *referring_type = ((TypeDeclaration*)referring_decl)->type_spec_;
    //if (referring_type->GetType() != ANT_CLASS_TYPE) return(nullptr);

    // forward referencing a class
    for (int ii = current_; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[current_];
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            AstNodeType spec_type = tdecl->type_spec_->GetType();
            if (spec_type == ANT_CLASS_TYPE && strcmp(tdecl->name_.c_str(), name) == 0) {
                if (!declaration->IsPublic() && current_is_public_) {
                    Error("A public declaration cannot refer a private symbol", location);
                    return(nullptr);
                }
                if (ii != current_) {
                    tdecl->SetForwardReferred(current_is_public_ ? FRT_PUBLIC : FRT_PRIVATE);
                }
                return(declaration);
            }
        }
    }
    return(nullptr);
}

int AstChecker::SearchAndLoadPackage(const char *name, IAstNode *location, const char *not_found_error_string)
{
    int len = root_->dependencies_.size();
    for (int ii = 0; ii < len; ++ii) {
        AstDependency *dependency = root_->dependencies_[ii];

        if (strcmp(name, dependency->package_name_.c_str()) == 0) {

            // found: mark it as used
            if (current_is_public_ && !in_function_block_) {
                dependency->SetUsage(DependencyUsage::PUBLIC);
            } else {
                dependency->SetUsage(DependencyUsage::PRIVATE);
            }

            if (dependency->ambiguous_) {
                Error("Local package name is ambiguous (matches two 'requires' declarations)", location);
                return(-1);
            }

            // find the related package descriptor
            int index = root_->dependencies_[ii]->package_index_;
            if (index >= (int)packages_->size() || index < 0) {
                break;  // should never happen, treat it like a not found package
            }
            Package *pkg = (*packages_)[index];
            bool    loaded = pkg->status_ >= PkgStatus::FOR_REFERENCIES;

            // if not already loaded and never tryed, try loading now !
            if (!loaded && pkg->status_ != PkgStatus::ERROR) {
                if (pkg->Load(PkgStatus::FOR_REFERENCIES)) {
                    AstChecker  checker;

                    if (checker.CheckAll(packages_, options_, index, false)) {
                        loaded = true;
                    } else {
                        pkg->SetError();
                    }
                }
            }

            // ops !!
            if (!loaded) {
                Error("Failed to load the package", location);
                return(-1);
            }
            return(index);
        }
    }
    Error(not_found_error_string, location);
    return(-1);
}

// if is_private is set to true the symbol is private (regrdless the return value)
// if is_private is false and the return is nullptr the symbol wasn't found
IAstDeclarationNode *AstChecker::SearchExternDeclaration(int package_index, const char *name, bool *is_private)
{
    *is_private = false;
    if (package_index >= (int)packages_->size() || package_index < 0) {
        return(nullptr);
    }
    Package *pkg = (*packages_)[package_index];
    IAstDeclarationNode *node = pkg->symbols_.FindDeclaration(name);
    if (node != nullptr) {
        *is_private = !node->IsPublic();
    } else if (pkg->root_->private_symbols_.LinearSearch(name) >= 0) {
        *is_private = true;
    }
    return(node);
}

bool AstChecker::FlagLocalVariableAsPointed(IAstExpNode *node)
{
    if (node != nullptr && node->GetType() == ANT_EXP_LEAF) {
        AstExpressionLeaf *leaf = (AstExpressionLeaf*)node;
        if (leaf->subtype_ == TOKEN_NAME) {
            IAstDeclarationNode *decl = symbols_->FindLocalDeclaration(leaf->value_.c_str());
            if (decl != nullptr) {
                AstNodeType nodetype = decl->GetType();
                if (nodetype == ANT_VAR) {
                    ((VarDeclaration*)decl)->SetFlags(VF_ISPOINTED);
                    return(true);
                }
            }
        }
    }
    return(false);
}

bool AstChecker::IsGoodForIndex(IAstDeclarationNode *declaration)
{
    if (declaration == nullptr) return(false);
    if (declaration->GetType() != ANT_VAR) return(false);
    VarDeclaration *var = (VarDeclaration*)declaration;
    return (var->HasOneOfFlags(VF_ISFORINDEX) && !var->HasOneOfFlags(VF_ISBUSY));
}

bool AstChecker::CanAssign(ExpressionAttributes *dst, ExpressionAttributes *src, IAstNode *err_location)
{
    string error;

    if (!dst->CanAssign(src, this, &error)) {
        Error(error.c_str(), err_location);
        return(false);
    }
    return(true);
}

bool AstChecker::CheckArrayIndicesInTypes(AstArrayType *array, TypeSpecCheckMode mode)
{
    bool success = true;

    // don't do it twice
    if (array->dimension_was_computed_) {

        // silent. If there was an error has been already emitted
        return(true); 
    }
    array->dimension_was_computed_ = true;

    if (array->dimension_ == 0) {
        if (array->expression_ != nullptr) {
            size_t      value = 0;
            ExpressionAttributes attr;

            CheckExpression(array->expression_, &attr, ExpressionUsage::READ);
            if (!attr.IsOnError()) {
                if (!attr.IsAValidArraySize(&value)) {
                    Error("Array size must be '*' (for dynamic) or a positive integer value", array);
                    value = 0;
                    success = false;
                } else if (!VerifyIndexConstness(array->expression_)) {
                    Error("Array size must be a simple integer compile time constant. Its expression can't contain the ^, ~ operators.", array);
                    value = 0;
                    success = false;
                }
            }
            array->dimension_ = value;
        } else if (!array->is_dynamic_ && mode != TSCM_INITEDVAR) {
            Error("You can't omit the array size unless you are declaring a variable or constant with an aggregate initializer.", array);
            success = false;
        }
    }
    return(success);
}

bool AstChecker::VerifyIndexConstness(IAstExpNode *node)
{
    switch (node->GetType()) {
    case ANT_BINOP:
        return(VerifyBinopForIndexConstness((AstBinop*)node));
    case ANT_UNOP:
        return(VerifyUnopForIndexConstness((AstUnop*)node));
    case ANT_EXP_LEAF:
        return(VerifyLeafForIndexConstness((AstExpressionLeaf*)node));
    default:
        break;
    }
    return(false);
}

bool AstChecker::VerifyBinopForIndexConstness(AstBinop *node)
{
    if (node->subtype_ == TOKEN_DOT) {
        const ExpressionAttributes *attr = node->GetAttr();
        if (attr != nullptr && attr->IsEnum() && !attr->IsAVariable() && attr->HasKnownValue()) {
            return(true);
        }
    }
    if (!VerifyIndexConstness(node->operand_left_) || !VerifyIndexConstness(node->operand_right_)) {
        return(false);
    }
    switch (node->subtype_) {
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_MOD:
    case TOKEN_SHR:
    case TOKEN_SHL:
    case TOKEN_AND:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_OR:
    case TOKEN_XOR:
    case TOKEN_DOT:
        return(true);
    default:
        break;
    }
    return(false);
}

bool AstChecker::VerifyUnopForIndexConstness(AstUnop *node)
{
    if (!VerifyIndexConstness(node->operand_)) {
        return(false);
    }
    switch (node->subtype_) {
    case TOKEN_INT8:
    case TOKEN_INT16:
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT8:
    case TOKEN_UINT16:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_MINUS:
    case TOKEN_PLUS:
        return(true);
    default:
        break;
    }
    return(false);
}

bool AstChecker::VerifyLeafForIndexConstness(AstExpressionLeaf *node)
{
    switch (node->subtype_) {
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_LITERAL_UINT:
        return(true);
    case TOKEN_NAME:
        if (node->pkg_index_ >= 0) {
            return(true);
        }
        if (node->wp_decl_ != nullptr) {
            IAstDeclarationNode *decl = node->wp_decl_;
            if (decl->GetType() == ANT_VAR) {
                VarDeclaration *var = (VarDeclaration*)decl;
                if (var->HasOneOfFlags(VF_IMPLEMENTED_AS_CONSTINT)) {
                    return(true);
                }
            } else if (decl->GetType() == ANT_TYPE) {
                TypeDeclaration *tdecl = (TypeDeclaration*)decl;
                IAstTypeNode *tspec = SolveTypedefs(tdecl->type_spec_);
                if (tspec == nullptr || tspec->GetType() == ANT_ENUM_TYPE) {
                    return(true);
                }
            }
        }
        break;
    default:
        break;
    }
    return(false);
}

bool AstChecker::IsCompileTimeConstant(IAstExpNode *node)
{
    switch (node->GetType()) {
    case ANT_BINOP:
        return(IsBinopCompileTimeConstant((AstBinop*)node));
    case ANT_UNOP:
        return(IsUnopCompileTimeConstant((AstUnop*)node));
    case ANT_EXP_LEAF:
        return(IsLeafCompileTimeConstant((AstExpressionLeaf*)node));
    default:
        break;
    }
    return(false);
}

bool AstChecker::IsBinopCompileTimeConstant(AstBinop *node)
{
    if (node->subtype_ == TOKEN_DOT) {
        const ExpressionAttributes *attr = node->GetAttr();
        return(attr != nullptr && attr->IsEnum() && !attr->IsAVariable() && attr->HasKnownValue());
    }
    if (!IsCompileTimeConstant(node->operand_left_) || !IsCompileTimeConstant(node->operand_right_)) {
        return(false);
    }
    switch (node->subtype_) {
    case TOKEN_MPY:
    case TOKEN_DIVIDE:
    case TOKEN_MOD:
    case TOKEN_SHR:
    case TOKEN_SHL:
    case TOKEN_AND:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_OR:
    case TOKEN_XOR:
    case TOKEN_POWER:
    case TOKEN_ANGLE_OPEN_LT:
    case TOKEN_ANGLE_CLOSE_GT:
    case TOKEN_GTE:
    case TOKEN_LTE:
    case TOKEN_DIFFERENT:
    case TOKEN_EQUAL:
    case TOKEN_LOGICAL_AND:
    case TOKEN_LOGICAL_OR:
        return(true);
    default:
        break;
    }
    return(false);
}

bool AstChecker::IsUnopCompileTimeConstant(AstUnop *node)
{
    if (!IsCompileTimeConstant(node->operand_)) {
        return(false);
    }
    switch (node->subtype_) {
    case TOKEN_INT8:
    case TOKEN_INT16:
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT8:
    case TOKEN_UINT16:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_MINUS:
    case TOKEN_PLUS:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
    case TOKEN_STRING:
    case TOKEN_NOT:
    case TOKEN_LOGICAL_NOT:
        return(true);
    default:
        break;
    }
    return(false);
}

bool AstChecker::IsLeafCompileTimeConstant(AstExpressionLeaf *node)
{
    switch (node->subtype_) {
    case TOKEN_NULL:
    case TOKEN_FALSE:
    case TOKEN_TRUE:
    case TOKEN_LITERAL_STRING:
    case TOKEN_LITERAL_UINT:
    case TOKEN_LITERAL_FLOAT:
    case TOKEN_LITERAL_IMG:

    // Casted
    case TOKEN_INT32:
    case TOKEN_INT64:
    case TOKEN_UINT32:
    case TOKEN_UINT64:
    case TOKEN_FLOAT32:
    case TOKEN_FLOAT64:
    case TOKEN_COMPLEX64:
    case TOKEN_COMPLEX128:
        return(true);
    case TOKEN_NAME:
        return (VerifyLeafForIndexConstness(node));
    default:
        break;
    }
    return(false);
}

//
// NOTE: if comparison is not for EQUALITY, t0 must be a destination, t1 a source
//
ITypedefSolver::TypeMatchResult AstChecker::AreTypeTreesCompatible(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode)
{
    AstArrayType    *array0;
    AstArrayType    *array1;
    TypeMatchResult result;

    t0 = SolveTypedefs(t0);
    t1 = SolveTypedefs(t1);
    if (t0 == nullptr || t1 == nullptr) {
        return(KO);
    }

    // some classes are not copyable
    if (mode == FOR_ASSIGNMENT && t1->GetType() == ANT_CLASS_TYPE) {
        if (!((AstClassType*)t0)->can_be_copied) {
            return(NONCOPY);
        }
    }

    // optimization (check the pointers instead of the content)
    if (t0 == t1) return(OK);

    AstNodeType t0type = t0->GetType();
    AstNodeType t1type = t1->GetType();

    // cases in which the types don't need to be the same: concrete to interface reference/pointer.
    if (mode != FOR_EQUALITY && AreInterfaceAndClass(t0, t1, mode)) {
        if (mode == FOR_ASSIGNMENT && t0type == ANT_POINTER_TYPE) {
            if (!((AstPointerType*)t0)->CheckConstness(t1, mode)) {
                return(CONST);
            }
            return(OK);
        } else if (mode == FOR_REFERENCING && t0type != ANT_POINTER_TYPE) {
            return(OK);
        }
    }

    if (t0type != t1type) return(KO);
    if (t0type == ANT_ARRAY_TYPE) {
        array0 = (AstArrayType*)t0;
        array1 = (AstArrayType*)t1;

        assert(array0->dimension_was_computed_);
        assert(array1->dimension_was_computed_);
        //CheckArrayIndicesInTypes(array0);
        //CheckArrayIndicesInTypes(array1);
    }
    if (!t0->IsCompatible(t1, mode)) {
        return(KO);
    }
    switch (t0type) {
    case ANT_ARRAY_TYPE:
        return (AreTypeTreesCompatible(array0->element_type_, array1->element_type_, FOR_EQUALITY));
    case ANT_MAP_TYPE:
        {
            AstMapType *map0 = (AstMapType*)t0;
            AstMapType *map1 = (AstMapType*)t1;
            result = AreTypeTreesCompatible(map0->key_type_, map1->key_type_, FOR_EQUALITY);
            if (result != OK) return(result);
            result = AreTypeTreesCompatible(map0->returned_type_, map1->returned_type_, FOR_EQUALITY);
            if (result != OK) return(result);
        }
        break;
    case ANT_POINTER_TYPE:
        result = AreTypeTreesCompatible(((AstPointerType*)t0)->pointed_type_, ((AstPointerType*)t1)->pointed_type_, FOR_EQUALITY);
        if (result != OK) return(result);
        if (!((AstPointerType*)t0)->CheckConstness(t1, mode)) {
            return(CONST);
        }
        break;
    case ANT_FUNC_TYPE:
        {
            AstFuncType *func0 = (AstFuncType*)t0;
            AstFuncType *func1 = (AstFuncType*)t1;
            result = AreTypeTreesCompatible(func0->return_type_, func1->return_type_, FOR_EQUALITY);
            if (result != OK) return(result);
            for (int ii = 0; ii < (int)func0->arguments_.size(); ++ii) {
                VarDeclaration *arg0 = func0->arguments_[ii];
                VarDeclaration *arg1 = func1->arguments_[ii];
                if ((arg0->flags_ & (VF_READONLY | VF_WRITEONLY)) != (arg1->flags_ & (VF_READONLY | VF_WRITEONLY))) {
                    return(KO);
                }
                result = AreTypeTreesCompatible(arg0->weak_type_spec_, arg1->weak_type_spec_, FOR_EQUALITY);
                if (result != OK) return(result);
            }
        }
        break;
    default:
        break;
    }
    return(OK);
}

bool AstChecker::AreInterfaceAndClass(IAstTypeNode *t0, IAstTypeNode *t1, TypeComparisonMode mode)
{ 
    if (t0->GetType() == ANT_POINTER_TYPE) {

        // must be both pointers, also checks weakness compatiblity based on mode
        if (!t0->IsCompatible(t1, mode)) {
            return(false);
        }
        t0 = SolveTypedefs(((AstPointerType*)t0)->pointed_type_);
        t1 = SolveTypedefs(((AstPointerType*)t1)->pointed_type_);
        if (t0 == nullptr || t1 == nullptr) {
            return(false);
        }
    }
    if (t0->GetType() != ANT_INTERFACE_TYPE) {
        return(false);
    }
    if (t1->GetType() == ANT_INTERFACE_TYPE) {
        return(((AstInterfaceType*)t1)->HasInterface((AstInterfaceType*)t0));
    } else if (t1->GetType() == ANT_CLASS_TYPE) {
        return(((AstClassType*)t1)->HasInterface((AstInterfaceType*)t0));
    }
    return(false);
}

bool AstChecker::NodeIsConcrete(IAstTypeNode *tt)
{
    if (tt == nullptr) return(true);    // silently ignore the error (probably an error was already emitted).
    switch (tt->GetType()) {
    case ANT_INTERFACE_TYPE:
        return(false);
    case ANT_NAMED_TYPE:
        return(NodeIsConcrete(SolveTypedefs(tt)));
    default:
        return(true);
    }
}

//
// We look for a return who stops the execution before the function ends with the default return (and no return value).
//
// to be effective the return doesn't need to be at the end position (if it is not, then it is followed by dead code)
// to be effective the return must be in the main block or in ALL the blocks of an if.
// If there is a while(true) then we know the default return is never executed and we are satisfied.
// We give up analizing other for and while loops. We assume they are not guaranteed to return directly.
//
bool AstChecker::BlockReturnsExplicitly(AstBlock *block)
{
    int         ii, jj;
    AstIf       *ifblock;
    IAstExpNode *exp;

    // if we find a return in the block - in any position - the execution can't reach the end.
    // if we don't, there is still a chance the execution can't reach the end if:
    // - the return is in an unconditioned block.
    // - a while(true) prevents the execution of the downstream code.
    // - all the clauses of an if (including the default one, which must be present) have returns.
    for (ii = (int)block->block_items_.size() - 1; ii >= 0; --ii) {
        IAstNode *node = block->block_items_[ii];
        switch (node->GetType()) {
        case ANT_BLOCK:
            if (BlockReturnsExplicitly((AstBlock*)node)) {
                return(true);
            }
            break;
        case ANT_WHILE:
            // If never ends prevents the default return execution, else we continue searching for an upstream return
            exp = ((AstWhile*)node)->expression_;
            if (exp->GetType() == ANT_EXP_LEAF && ((AstExpressionLeaf*)exp)->subtype_ == TOKEN_TRUE) {
                return(true);
            }
            break;
        case ANT_IF:
            ifblock = (AstIf*)node;
            if (ifblock->default_block_ != nullptr &&  BlockReturnsExplicitly(ifblock->default_block_)) {
                for (jj = 0; jj < (int)ifblock->blocks_.size(); ++jj) {
                    if (!BlockReturnsExplicitly(ifblock->blocks_[jj])) {
                        break;
                    }
                }
                if (jj == ifblock->blocks_.size()) {
                    return(true);   // all the possible if choices are properly terminated
                }
            }
            // else we go on in the hope this ifblock is dead code (a return is upstream).
            break;
        case ANT_RETURN:
            return(true);
        default:
            break;
        }
    }
    return(false);
}

void AstChecker::CheckIfVarReferenceIsLegal(VarDeclaration *var, IAstNode *location)
{
    if (in_function_block_ && current_function_->function_type_->ispure_) {
        if (!var->HasOneOfFlags(VF_READONLY | VF_ISPOINTED | VF_ISARG | VF_ISFORINDEX | VF_ISFORITERATOR | VF_ISLOCAL)) {
            Error("A pure function can only access local variables !!", location);
            return;
        }
    }
    if (var->HasOneOfFlags(VF_IS_ITERATED)) {
        Error("An Iterated variable can be accessed ONLY through the iterator. (to avoid buffer reallocations)", location);
        return;
    }
}

void AstChecker::CheckIfFunCallIsLegal(AstFuncType *func, IAstNode *location)
{
    if (in_function_block_ && current_function_->function_type_->ispure_ && !func->ispure_) {
        Error("A pure function can only call pure functions !!", location);
    }
}

VarDeclaration *AstChecker::GetIteratedVar(IAstExpNode *node)
{
    switch (node->GetType()) {
    case ANT_INDEXING:
        return(GetIteratedVar(((AstIndexing*)node)->indexed_term_));
    case ANT_FUNCALL:
        return(nullptr);    // shouldn't happen
    case ANT_BINOP:
        if (((AstBinop*)node)->subtype_ == TOKEN_DOT) {
            AstBinop *dotop = (AstBinop*)node;
            VarDeclaration *decl = GetIteratedVar(dotop->operand_left_);
            if (decl != nullptr) return(decl);
            return(GetIteratedVar(dotop->operand_right_));
        }
        break;
    case ANT_UNOP:
        return(nullptr);    // the only legal case is if it comes from a pointer.
    case ANT_EXP_LEAF:
        {
            IAstDeclarationNode *decl = ((AstExpressionLeaf*)node)->wp_decl_;
            if (decl != nullptr && decl->GetType() == ANT_VAR) {
                return((VarDeclaration*)decl);
            }
        }
        break;
    }
    return(nullptr);
}

void AstChecker::CheckInnerBlockVarUsage(void)
{
    int idx = 0;

    while (true) {
        IAstDeclarationNode *decl = symbols_->EnumerateInnerDeclarations(idx);
        if (decl == nullptr) break;
        idx++;
        if (decl->GetType() == ANT_VAR) {
            CheckPrivateVarUsage((VarDeclaration*)decl);
        }
    }
}

void AstChecker::CheckPrivateDeclarationsUsage(void)
{
    int idx = 0;

    while (true) {
        IAstDeclarationNode *decl = symbols_->EnumerateGlobalDeclarations(idx);
        if (decl == nullptr) break;
        idx++;
        if (decl->IsPublic()) continue;
        if (decl->GetType() == ANT_VAR) {
            CheckPrivateVarUsage((VarDeclaration*)decl);
        } else if (decl->GetType() == ANT_FUNC) {
            CheckPrivateFuncUsage((FuncDeclaration*)decl);
        } else if (decl->GetType() == ANT_TYPE) {
            CheckPrivateTypeUsage((TypeDeclaration*)decl);
        }
    }
}

void AstChecker::CheckPrivateVarUsage(VarDeclaration *var, bool is_member)
{
    bool check_all = !is_member && IsArgTypeEligibleForAnIniter(var->weak_type_spec_);
    if (!var->HasOneOfFlags(VF_ISPOINTED | VF_ISARG | VF_ISFORINDEX | VF_IS_REFERENCE | VF_ISFORITERATOR | VF_IMPLEMENTED_AS_CONSTINT)) {
        if (!var->HasOneOfFlags(VF_WASREAD | VF_WASWRITTEN)) {
            UsageError("Variable/Constant unused !!", var);
        } else if (check_all && !var->HasOneOfFlags(VF_READONLY) && !var->HasOneOfFlags(VF_WASWRITTEN)) {
            UsageError("Variable never written, please declare as a constant !!", var);
        }
    } else if (check_all && var->HasAllFlags(VF_ISARG | VF_WRITEONLY)) {
        if (var->HasOneOfFlags(VF_WASREAD) && !var->HasOneOfFlags(VF_WASWRITTEN)) {
            UsageError("Output parameter read before being written !!", var);
        }
    }
}

void AstChecker::CheckPrivateFuncUsage(FuncDeclaration *func)
{
    if (!func->is_used_) {
        UsageError("Function is unused !!", func);
    }
}

void AstChecker::CheckPrivateTypeUsage(TypeDeclaration *tdec)
{
    if (!tdec->is_used_) {
        UsageError("Type is unused !!", tdec);
    }
    if (tdec->type_spec_->GetType() == ANT_CLASS_TYPE) {
        AstClassType *ctype = (AstClassType*)tdec->type_spec_;
        for (int ii = 0; ii < (int)ctype->member_vars_.size(); ++ii) {
            VarDeclaration *var = ctype->member_vars_[ii];
            if (!var->IsPublic()) CheckPrivateVarUsage(var, true);
        }
        for (int ii = 0; ii < (int)ctype->member_functions_.size() && ii < ctype->first_hinherited_member_; ++ii) {
            FuncDeclaration *func = ctype->member_functions_[ii];
            if (!func->IsPublic()) CheckPrivateFuncUsage(func);
        }
    }
}

void AstChecker::CheckMemberFunctionsDeclarationsPresence(void)
{
    int idx = 0;

    while (true) {
        IAstDeclarationNode *decl = symbols_->EnumerateGlobalDeclarations(idx);
        if (decl == nullptr) break;
        idx++;
        if (decl->GetType() == ANT_TYPE) {
            TypeDeclaration *tdec = (TypeDeclaration*)decl;
            if (tdec->type_spec_->GetType() == ANT_CLASS_TYPE) {
                AstClassType *ctype = (AstClassType*)tdec->type_spec_;
                for (int ii = 0; ii < (int)ctype->member_functions_.size(); ++ii) {
                    if (!ctype->implemented_[ii]) {
                        if (ii < ctype->first_hinherited_member_) {
                            Error("Function has not an implementation", ctype->member_functions_[ii]);
                        } else {
                            string message = "Inherited Function \"";
                            message += ctype->member_functions_[ii]->name_;
                            message += "\" has not an implementation";
                            Error(message.c_str(), ctype, true);
                        }
                    }
                }
            }
        }
    }
}

bool AstChecker::IsArgTypeEligibleForAnIniter(IAstTypeNode *type)
{
    type = SolveTypedefs(type);
    if (type == nullptr) return(true);  // silent (there is an error elsewhere)
    switch (type->GetType()) {
    case ANT_BASE_TYPE:
    case ANT_POINTER_TYPE:
    case ANT_FUNC_TYPE:
    case ANT_ENUM_TYPE:
        return(true);
    default:
        break;
    }
    return(false);
}

AstClassType *AstChecker::GetLocalClassTypeDeclaration(const char *classname, bool solve_typedefs)
{
    IAstDeclarationNode *node = symbols_->FindDeclaration(classname);
    if (node != nullptr && node->GetType() == ANT_TYPE) {
        IAstTypeNode *ntype = ((TypeDeclaration*)node)->type_spec_;
        if (solve_typedefs) {
            ntype = SolveTypedefs(ntype);
        }
        if (ntype != nullptr && ntype->GetType() == ANT_CLASS_TYPE) {
            return((AstClassType*)ntype);
        }
    }
    return(nullptr);
}

FuncDeclaration *AstChecker::SearchFunctionInClass(AstClassType *the_class, const char *name)
{
    if (the_class != nullptr) {
        for (int ii = 0; ii < (int)the_class->member_functions_.size(); ++ii) {
            if (the_class->member_functions_[ii]->name_ == name) {
                return(the_class->member_functions_[ii]);
            }
        }
    }
    return(nullptr);
}

void AstChecker::Error(const char *message, IAstNode *location, bool use_last_location)
{
    //char    fullmessage[600];
    int     row, col;

    if (use_last_location) {
        row = location->GetPositionRecord()->last_row;
        col = location->GetPositionRecord()->last_col;
    } else {
        row = location->GetPositionRecord()->start_row;
        col = location->GetPositionRecord()->start_col;
    }
    errors_->AddError(message, row, col + 1);
    //if (strlen(message) > 512) {
    //    errors_->AddName(message);
    //} else {
    //    sprintf(fullmessage, "line: %d \tcolumn: %d \t%s", row, col + 1, message);
    //    errors_->AddName(fullmessage);
    //}
}

void AstChecker::UsageError(const char *message, IAstNode *location, bool use_last_location)
{
    //char    fullmessage[600];
    int     row, col;

    if (use_last_location) {
        row = location->GetPositionRecord()->end_row;
        col = location->GetPositionRecord()->end_col;
    } else {
        row = location->GetPositionRecord()->start_row;
        col = location->GetPositionRecord()->start_col;
    }
    usage_errors_.AddError(message, row, col + 1);

    //if (strlen(message) > 512) {
    //    usage_errors_.AddName(message);
    //} else {
    //    sprintf(fullmessage, "line: %d \tcolumn: %d \t%s", row, col + 1, message);
    //    usage_errors_.AddName(fullmessage);
    //}
}

} // namespace