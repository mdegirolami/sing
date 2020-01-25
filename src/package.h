#ifndef PACKAGE_H
#define PACKAGE_H

#include "NamesList.h"
#include "ast_nodes.h"
#include "symbols_storage.h"
#include "options.h"

namespace SingNames {

// after ERROR, keep these in order of increasing completion
enum class PkgStatus { UNLOADED, ERROR, HEADER_ONLY, FOR_REFERENCIES, FULL };

class Package {
public:
    AstFile         *root_;
    SymbolsStorage  symbols_;
    NamesList       errors_;
    string          fullpath_;      // inclusive of search path
    PkgStatus       status_;

    Package();
    ~Package();

    void Init(const char *filename);
    bool Load(PkgStatus wanted_status);
    const char *GetError(int index) { return(errors_.GetName(index)); }
    bool HasErrors(void) { return(errors_.Size() > 0); }
    void SetError(void) { status_ = PkgStatus::ERROR; }
};

} // namespace

#endif