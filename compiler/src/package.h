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
                        HEADER_ONLY,        // unused value
                        FOR_REFERENCIES,    // loaded because referenced - fun. bodies and private functions are not parsed
                        FULL };             // loaded to be compiled.

class Package {
public:
    ErrorList       errors_;
    AstFile         *root_;
    SymbolsStorage  symbols_;
    string          fullpath_;      // inclusive of search path
    PkgStatus       status_;

    Package();
    ~Package();

    void Init(const char *filename);
    bool Load(PkgStatus wanted_status);
    const char *GetError(int index);
    bool HasErrors(void) { return(errors_.NumErrors() > 0); }
    void SetError(void) { status_ = PkgStatus::ERROR; }
    void SortErrors(void) { errors_.Sort(); }
};

} // namespace

#endif