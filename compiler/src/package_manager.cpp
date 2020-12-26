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

int PackageManager::init_pkg(const char *name, bool force_init)
{
    // existing ?
    for (int ii = 0; ii < (int)packages_.size(); ++ii) {
        if (is_same_filename(packages_[ii]->getFullPath(), name)) {
            if (force_init) {
                onInvalidation(ii);
                packages_[ii]->Init(name);
            }
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
        return(pkg->advanceTo(wanted_status));
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

void PackageManager::applyPatch(int index, int start, int stop, const char *new_text)
{
    Package *pkg = pkgFromIdx(index);
    if (pkg == nullptr) return;
    onInvalidation(index);
    pkg->applyPatch(start, stop, new_text);
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
    // existing ?
    for (int ii = 0; ii < (int)packages_.size(); ++ii) {
        if (is_same_filename(packages_[ii]->getFullPath(), name)) {

            // revert to unloaded
            onInvalidation(ii);
            packages_[ii]->Init(name);
        }
    }
}

} // namespace