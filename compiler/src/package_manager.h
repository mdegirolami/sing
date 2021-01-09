#ifndef PACKAGE_MGR_H
#define PACKAGE_MGR_H

#include "package.h"
#include "vector.h"
#include "ast_nodes.h"
#include "options.h"

namespace SingNames {

class PackageManager {
    vector<Package*> packages_;
    Options         *options_;
    int             main_package_; // the package we are trying to check now (fully loaded, under check)  

    Package *pkgFromIdx(int index) const;
    void    onInvalidation(int index);
public:
    void        init(Options *options) { options_ = options; main_package_ = -1; }

    int         init_pkg(const char *name, bool force_init = false);
    bool        load(int index, PkgStatus wanted_status);
    bool        check(int index, bool is_main);

    bool        isMainIndex(int index) { return(index != -1 && index == main_package_); }
    int         getPkgsNum(void) const { return(packages_.size()); }
    IAstDeclarationNode *findSymbol(int index, const char *name, bool *is_private);
    PkgStatus   getStatus(int index) const;
    const Package *getPkg(int index) const;
    void        applyPatch(int index, int from_row, int from_col, int to_row, int to_col, int allocate, const char *newtext);
    void        insertInSrc(int index, const char *newtext);
    void        on_deletion(const char *name);
};

} // namespace

#endif