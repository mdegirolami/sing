#include "package_manager.h"
#include "ast_checks.h"
#include "FileName.h"
#include "Parser.h"
#include "builtin_functions.h"

namespace SingNames {

PackageManager::~PackageManager()
{
    for (int ii = 0; ii < packages_.size(); ++ii) {
        if (packages_[ii] != nullptr) {
            delete packages_[ii];
        }
    }
}

Package *PackageManager::pkgFromIdx(int index) const
{
    if (index < (int)packages_.size() && index >= 0) {
        const Package *pkg = packages_[index];
        return((Package*)pkg);
    }
    return(nullptr);
}

int PackageManager::init_pkg(const char *name, bool force_init)
{
    // normalize to make comparable
    string fullpath = name;
    FileName::Normalize(&fullpath);

    // existing ?
    for (int ii = 0; ii < (int)packages_.size(); ++ii) {
        if (is_same_filename(packages_[ii]->getFullPath(), fullpath.c_str())) {
            if (force_init) {
                onInvalidation(ii);
                packages_[ii]->Init(fullpath.c_str(), ii);
            }
            return(ii);
        }
    }

    // if not found
    int index = (int)packages_.size();
    Package *pkg = new Package;
    pkg->Init(fullpath.c_str(), index);
    packages_.push_back(pkg);
    return(index);
}

bool PackageManager::load(int index, PkgStatus wanted_status)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) {
        return(pkg->advanceTo(wanted_status, options_->ServerMode()));
    }
    return(false);
}

bool PackageManager::check(int index, bool is_main)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr){                
        AstChecker  checker;
        checker.init(this, options_, index);
        if (is_main) main_package_ = index;
        bool result = pkg->check(&checker);
        if (is_main) main_package_ = -1;
        return(result);
    }
    return(false);
}

IAstDeclarationNode *PackageManager::findSymbol(int index, const char *name, bool *is_private)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) return(pkg->findSymbol(name, is_private));
    *is_private = false;
    return(nullptr);
}

const Package *PackageManager::getPkg(int index) const
{
    return(pkgFromIdx(index));
}

PkgStatus PackageManager::getStatus(int index) const
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) return(pkg->getStatus());
    return(PkgStatus::ERROR);
}

void PackageManager::applyPatch(int index, int from_row, int from_col, int to_row, int to_col, int allocate, const char *newtext)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return;
    onInvalidation(index);
    pkg->applyPatch(from_row, from_col, to_row, to_col, allocate, newtext);
}

void PackageManager::insertInSrc(int index, const char *newtext)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return;
    onInvalidation(index);
    pkg->insertInSrc(newtext);
}

void PackageManager::onInvalidation(int index)
{
    vector<int> stack;

    // apply patch - this reverts to LOADED state
    Package *pkg = packages_[index];
    if (pkg == nullptr) return;
    PkgStatus status = pkg->getStatus();
    if (status == PkgStatus::FULL || status == PkgStatus::FOR_REFERENCIES) {
        stack.push_back(index);
    }

    // files who depend on the reset packages must be reset too
    while (stack.size() > 0) {
        int to_check = stack[stack.size() - 1];
        stack.pop_back();
        for (int ii = 0; ii < packages_.size(); ++ii) {
            if (ii == to_check) continue;
            pkg = packages_[ii];
            if (pkg != nullptr && pkg->depends_from(to_check)) {
                status = pkg->getStatus();
                if (status == PkgStatus::FULL || status == PkgStatus::FOR_REFERENCIES) {
                    stack.push_back(ii);
                    pkg->clearParsedData();
                }
            }
        }
    }
}

void PackageManager::on_deletion(const char *name)
{
    // normalize to make comparable
    string fullpath = name;
    FileName::Normalize(&fullpath);

    // existing ?
    for (int ii = 0; ii < (int)packages_.size(); ++ii) {
        if (is_same_filename(packages_[ii]->getFullPath(), fullpath.c_str())) {

            // revert to unloaded
            onInvalidation(ii);
            packages_[ii]->Init(fullpath.c_str(), ii);
        }
    }
}

void PackageManager::getSuggestions(NamesList *names, int index, int row, int col, char trigger)
{
    names->Reset();
    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return;

    int argument_index = -1;
    if (trigger == '"' || trigger == '/') {
        string path;
        if (pkg->GetPartialPath(&path, row, col)) {
            options_->GetAllFilesIn(names, path.c_str());
        }
        return;
    } else if (trigger == ':') {
        argument_index = pkg->SearchFunctionStart(&row, &col);
        if (argument_index == -1) {
            return;
        } else {
            trigger = '(';
        }
    }

    // parse    
    CompletionHint  hint;
    hint.row = row;
    hint.col = col;
    hint.trigger = trigger;
    AstChecker checker;
    if (!parseAndCheckForSuggestions(&hint, pkg, index, &checker)) {
        return;
    }

    switch (hint.type) {
    case CompletionType::TAG:
        {
            int ext_pkg = checker.SearchAndLoadPackage(hint.tag.c_str(), nullptr, nullptr);
            if (ext_pkg != -1) {
                Package *xpkg = pkgFromIdx(ext_pkg);
                if (xpkg != nullptr) xpkg->getAllPublicTypeNames(names);
            }
        }
        break;
    case CompletionType::FUNCTION:
        {
            IAstDeclarationNode *decl = pkg->getDeclaration(hint.tag.c_str());
            if (decl == nullptr) return;
            if (decl->GetType() != AstNodeType::ANT_TYPE) return;
            IAstTypeNode *ctype = ((TypeDeclaration*)decl)->type_spec_;
            if (ctype == nullptr) return;
            if (ctype->GetType() != AstNodeType::ANT_CLASS_TYPE) return;
            ((AstClassType*)ctype)->getFunctionsNames(names, true, true);
        }
        break;
    case CompletionType::OP:
        if (hint.node != nullptr) {
            if (trigger == '.' && hint.node->GetType() == ANT_BINOP) {
                getSuggestionsForDotInExpression(names, (AstBinop*)hint.node, pkg);
            } else if (trigger == '(') {
                const ExpressionAttributes *attr = hint.node->GetAttr();
                const AstFuncType *ft = attr->GetFunCallType();
                if (argument_index < ft->arguments_.size()) {
                    names->AddName(ft->arguments_[argument_index]->name_.c_str());
                }
            }
        }
        pkg->clearParsedData();
        break;
    }
}

void PackageManager::getSuggestionsForDotInExpression(NamesList *names, AstBinop *dotexp, Package *pkg)
{
    IAstExpNode *left = dotexp->operand_left_;
    if (left == nullptr) return;
    const ExpressionAttributes *attr = left->GetAttr();
    bool is_this = false;

    int pkg_index = -1;
    if (left->GetType() == ANT_EXP_LEAF) {
        pkg_index = ((AstExpressionLeaf*)left)->pkg_index_;
        is_this = ((AstExpressionLeaf*)left)->subtype_ == TOKEN_THIS;
    }
    if (pkg_index != -1) {
        // case 1: <file tag>.<extern_symbol>
        Package *extpkg = pkgFromIdx(pkg_index);
        if (extpkg != nullptr) extpkg->getAllPublicDeclNames(names);
    } else {
        if (attr->IsOnError()) return;
        bool try_buitins = true;

        if (attr->IsEnum()) {
            if (!attr->HasKnownValue() && !attr->IsAVariable()) {
                AstEnumType *enumnode = (AstEnumType*)attr->GetTypeTree();
                int entry;
                for (entry = 0; entry < (int)enumnode->items_.size(); ++entry) {
                    names->AddName(enumnode->items_[entry].c_str());
                }
            }
            try_buitins = false;
        } else {
            IAstTypeNode *thetype = attr->GetPointedType();
            if (thetype == nullptr) thetype = attr->GetTypeTree();
            if (thetype != nullptr) {
                if (thetype->GetType() == ANT_CLASS_TYPE) {
                    AstClassType *classnode = (AstClassType*)thetype;
                    classnode->getFunctionsNames(names, is_this, false);
                    classnode->getVariablesNames(names, is_this);
                    try_buitins = false;
                } else if (thetype->GetType() == ANT_INTERFACE_TYPE) {
                    AstInterfaceType *ifnode = (AstInterfaceType*)thetype;
                    ifnode->getFunctionsNames(names);
                    try_buitins = false;
                }
            }
        }
        if (try_buitins) {
            GetBuiltinNames(names, attr);
        }
    }
}

int PackageManager::getSignature(string *signature, int index, int row, int col, char trigger)
{
    int argument_index = 0;
    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return(-1);

    if (trigger == ',') {
        argument_index = pkg->SearchFunctionStart(&row, &col);
        if (argument_index == -1) {
            return(-1);
        }
    }

    // parse    
    CompletionHint  hint;
    hint.row = row;
    hint.col = col;
    hint.trigger = '(';
    AstChecker checker;
    if (!parseAndCheckForSuggestions(&hint, pkg, index, &checker)) {
        return(-1);
    }
    if (hint.type != CompletionType::OP || hint.node == nullptr) {
        pkg->clearParsedData();
        return(-1);
    }

    // analyze the return values
    const ExpressionAttributes *attr = hint.node->GetAttr();
    const AstFuncType *ft = attr->GetFunCallType();
    if (ft == nullptr) {
        pkg->clearParsedData();
        return(-1);
    }
    *signature = "";
    ft->SynthSingType(signature);

    pkg->clearParsedData();
    return(argument_index);
}

bool PackageManager::findSymbol(string *def_file, int *file_row, int *file_col, int index, int row, int col)
{
    bool need_to_clean = false;

    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return(false);

    IAstNode *node = findSymbolPos(pkg, index, row, col, &need_to_clean);
    if (node == nullptr) {
        if (need_to_clean) pkg->clearParsedData();
        return(false);
    }

    PositionInfo *pos = node->GetPositionRecord();
    if (pos == nullptr) {
        if (need_to_clean) pkg->clearParsedData();
        return(false);
    }

    Package *xpkg = pkgFromIdx(pos->package_idx);
    *def_file = xpkg->getFullPath();

    if (need_to_clean) pkg->clearParsedData();

    return(xpkg->ConvertPosition(pos, file_row, file_col));
}

IAstNode *PackageManager::findSymbolPos(Package *pkg, int index, int row, int col, bool *need_to_clean)
{
    *need_to_clean = false;

    // extract the symbol
    string symbol;
    int dot_row, dot_col;
    pkg->GetSymbolAt(&symbol, &dot_row, &dot_col, row, col);
    if (symbol.length() < 1) return(nullptr);

    // unqualifyed symbol (local)
    if (dot_row == -1) {        
        pkg->advanceTo(PkgStatus::FULL, true);
        AstClassType *classnode;
        IAstDeclarationNode *decl;
        if (pkg->SymbolIsInMemberDeclaration(&classnode, &decl, symbol.c_str(), row, col)) {

            // the name in a member declaration in a class/interface.
            // if is a function declaration in a class, go to the definition.
            return(Declaration2Definition(decl, classnode, symbol.c_str()));
        } else {

            // all other cases (refs to local, automatic vars, for indices, etc..)
            return(pkg->getDeclarationReferredAt(symbol.c_str(), row, col));
        }
    }

    // qualifyed symbols
    // if preceeded by a dot, must know what the dot means to find the symbol context.
    CompletionHint  hint;
    hint.row = dot_row;
    hint.col = dot_col;
    hint.trigger = '.';
    AstChecker checker;
    if (!parseAndCheckForSuggestions(&hint, pkg, index, &checker)) {
        return(nullptr);
    }

    switch (hint.type) {
    case CompletionType::TAG:
        {
            int ext_pkg = checker.SearchAndLoadPackage(hint.tag.c_str(), nullptr, nullptr);
            if (ext_pkg != -1) {
                Package *xpkg = pkgFromIdx(ext_pkg);
                if (xpkg != nullptr) {
                    return(xpkg->getDeclaration(symbol.c_str()));
                }
            }
        }
        break;
    case CompletionType::FUNCTION:
        {
            IAstDeclarationNode *decl = pkg->getDeclaration(hint.tag.c_str());
            if (decl == nullptr) return(nullptr);
            if (decl->GetType() != AstNodeType::ANT_TYPE) return(nullptr);
            IAstTypeNode *ctype = ((TypeDeclaration*)decl)->type_spec_;
            if (ctype == nullptr) return(nullptr);
            if (ctype->GetType() != AstNodeType::ANT_CLASS_TYPE) return(nullptr);
            return(((AstClassType*)ctype)->getMemberDeclaration(symbol.c_str()));
        }
        break;
    case CompletionType::OP:
        *need_to_clean = true;
        if (hint.node != nullptr) {
            if (hint.node->GetType() == ANT_BINOP) {
                return(getQualifyedSymbolDefinition(symbol.c_str(), (AstBinop*)hint.node));
            }
        }
        break;
    }
    return(nullptr);
}

IAstNode *PackageManager::getQualifyedSymbolDefinition(const char *symbol, AstBinop *dotexp)
{
    IAstExpNode *left = dotexp->operand_left_;
    if (left == nullptr) return(nullptr);
    const ExpressionAttributes *attr = left->GetAttr();

    int pkg_index = -1;
    if (left->GetType() == ANT_EXP_LEAF) {
        pkg_index = ((AstExpressionLeaf*)left)->pkg_index_;
    }
    if (pkg_index != -1) {
        // case 1: <file tag>.<extern_symbol>
        Package *extpkg = pkgFromIdx(pkg_index);
        if (extpkg != nullptr) {
            return(extpkg->getDeclaration(symbol));
        }
    } else {
        if (attr->IsOnError()) return(nullptr);

        if (attr->IsEnum()) {
            if (!attr->HasKnownValue() && !attr->IsAVariable()) {
                return(attr->GetTypeTree());
           }
        } else {
            IAstTypeNode *thetype = attr->GetPointedType();
            if (thetype == nullptr) thetype = attr->GetTypeTree();
            if (thetype != nullptr) {
                if (thetype->GetType() == ANT_CLASS_TYPE) {
                    AstClassType *classnode = (AstClassType*)thetype;
                    IAstDeclarationNode *decl = classnode->getMemberDeclaration(symbol);
                    if (decl != nullptr) {

                        // try to find the definition (prefer over the declaration)
                        return(Declaration2Definition(decl, classnode, symbol));
                    }
                } else if (thetype->GetType() == ANT_INTERFACE_TYPE) {
                    AstInterfaceType *ifnode = (AstInterfaceType*)thetype;
                    return(ifnode->getMemberDeclaration(symbol));
                }
            }
        }
    }
    return(nullptr);
}

IAstNode *PackageManager::Declaration2Definition(IAstDeclarationNode *decl, AstClassType *classnode, const char *symbol)
{
    if (decl != nullptr && decl->GetType() == ANT_FUNC) {
        Package *pkg = pkgFromIdx(decl->GetPositionRecord()->package_idx);
        if (pkg != nullptr) {
            FuncDeclaration *def = pkg->findFuncDefinition(classnode, symbol);
            if (def != nullptr) {
                decl = def;
            }
        }
    }
    return(decl);
}

void PackageManager::getAllSymbols(vector<SymbolNfo> *vv, int index)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) pkg->getAllPublicNames(vv);
}

bool PackageManager::parseAndCheckForSuggestions(CompletionHint *hint, Package *pkg, int index, AstChecker *checker)
{
    pkg->parseForSuggestions(hint);
    if (hint->type == CompletionType::NOT_FOUND) {
        return(false);
    }

    // check
    checker->init(this, options_, index);
    if (hint->type == CompletionType::OP) {
        if (hint->node == nullptr) {
            pkg->clearParsedData();
            return(false);
        }
        checker->checkIfAstBlockChild(hint->node);
    }
    main_package_ = index;
    pkg->check(checker);
    main_package_ = -1;
    if (hint->type == CompletionType::OP && !checker->nodeWasFound()) {
        hint->type = CompletionType::NOT_FOUND;
        hint->node = nullptr;
        pkg->clearParsedData();
        return(false);
    }
    return(true);
}

} // namespace