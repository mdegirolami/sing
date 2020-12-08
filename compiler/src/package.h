#ifndef PACKAGE_H
#define PACKAGE_H

#include "NamesList.h"
#include "ast_nodes.h"
#include "symbols_storage.h"
#include "options.h"
#include "helpers.h"

namespace SingNames {

// after ERROR, keep these in order of increasing completion
enum class PkgStatus {  UNLOADED, 
                        ERROR,              // failed to load 
                        FOR_REFERENCIES,    // loaded because referenced - fun. bodies and private functions are not parsed
                        FULL };             // loaded to be compiled.

class AstChecker;

class Package {
    ErrorList       errors_;
    AstFile         *root_;
    SymbolsStorage  symbols_;
    string          fullpath_;      // inclusive of search path
    PkgStatus       status_;
    bool            checked_;

public:
    Package();
    ~Package();

    void Init(const char *filename);
    bool Load(PkgStatus wanted_status);
    bool check(AstChecker *checker);

    IAstDeclarationNode *findSymbol(const char *name, bool *is_private);
    PkgStatus getStatus(void) { return status_; }
    const char *getFullPath(void) const { return fullpath_.c_str(); }
    const AstFile *GetRoot(void) const { return(root_); }

    const char *GetError(int index) const;
    bool HasErrors(void) { return(errors_.NumErrors() > 0); }
    void SetError(void) { status_ = PkgStatus::ERROR; }
    void SortErrors(void) { errors_.Sort(); }
};

} // namespace

#endif