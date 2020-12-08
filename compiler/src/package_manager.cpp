#include "package_manager.h"
#include "ast_checks.h"

namespace SingNames {

Package *PackageManager::pkgFromIdx(int index) const
{
    if (index < (int)packages_.size() && index >= 0) {
        const Package *pkg = packages_[index];
        return((Package*)pkg);
    }
    return(nullptr);
}

int PackageManager::init_pkg(const char *name)
{
    // existing ?
    for (int ii = 0; ii < (int)packages_.size(); ++ii) {
        if (is_same_filename(packages_[ii]->getFullPath(), name)) {
            return(ii);
        }
    }

    // if not found
    int index = (int)packages_.size();
    Package *pkg = new Package;
    pkg->Init(name);
    packages_.push_back(pkg);
    return(index);
}

bool PackageManager::load(int index, PkgStatus wanted_status)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) {
        return(pkg->Load(wanted_status));
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

void PackageManager::setError(int index)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) pkg->SetError();
}

PkgStatus PackageManager::getStatus(int index) const
{
    Package *pkg = pkgFromIdx(index);
    if (pkg != nullptr) return(pkg->getStatus());
    return(PkgStatus::ERROR);
}

void PackageManager::applyPatch(int index, int start, int stop, const char *new_text)
{

}





} // namespace