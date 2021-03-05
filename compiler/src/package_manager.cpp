#include "package_manager.h"
#include "ast_checks.h"
#include "FileName.h"
#include "Parser.h"
#include "builtin_functions.h"

namespace SingNames {

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
                packages_[ii]->Init(fullpath.c_str());
            }
            return(ii);
        }
    }

    // if not found
    int index = (int)packages_.size();
    Package *pkg = new Package;
    pkg->Init(fullpath.c_str());
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
            packages_[ii]->Init(fullpath.c_str());
        }
    }
}

void PackageManager::getSuggestions(NamesList *names, int index, int row, int col, char trigger)
{
    names->Reset();
    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return;

    // parse    
    CompletionHint  hint;
    hint.row = row + 1;
    hint.col = col;
    hint.trigger = trigger;
    pkg->parseForSuggestions(&hint);
    if (hint.type == CompletionType::NOT_FOUND) {
        return;
    }

    // check
    AstChecker checker;
    checker.init(this, options_, index);
    main_package_ = index;
    pkg->check(&checker);
    main_package_ = -1;

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
            }
            pkg->clearParsedData();
        }
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

} // namespace