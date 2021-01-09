#ifndef PACKAGE_H
#define PACKAGE_H

#include "NamesList.h"
#include "ast_nodes.h"
#include "symbols_storage.h"
#include "options.h"
#include "helpers.h"
#include "split_vector.h"

namespace SingNames {

// DONT CHANGE THE ORDER: They are from less to more complete !
enum class PkgStatus {  UNLOADED,           // just inited
                        ERROR,              // failed to load 
                        LOADED,             // loaded, not parsed or checked
                        FOR_REFERENCIES,    // parsed and checked because referenced - fun. bodies and private functions are not parsed
                        FULL };             // fully parsed and checked.

class AstChecker;

static const int max_filesize = 10*1024*1024;

class Package {

    // from init
    string          fullpath_;      // inclusive of search path

    // from load
    SplitVector     source_;    

    // from parse and check
    ErrorList       errors_;
    AstFile         *root_;
    SymbolsStorage  symbols_;

    // status
    PkgStatus       status_;
    bool            checked_;

    bool Load(void);
    void SortErrors(void) { errors_.Sort(); }

public:
    Package();
    ~Package();

    void Init(const char *filename);    // reverts to UNLOADED
    void clearParsedData(void);         // reverts to LOADED
    bool advanceTo(PkgStatus wanted_status);
    bool check(AstChecker *checker);
    void applyPatch(int from_row, int from_col, int to_row, int to_col, int allocate, const char *newtext);
    void insertInSrc(const char *newtext);
    bool depends_from(int index);

    IAstDeclarationNode *findSymbol(const char *name, bool *is_private);
    PkgStatus getStatus(void) { return status_; }
    const char *getFullPath(void) const { return fullpath_.c_str(); }
    const AstFile *GetRoot(void) const { return(root_); }

    const char *GetErrorString(int index) const;
    const char *GetError(int index, int *row, int *col, int *endrow, int *endcol) const;
    bool HasErrors(void) { return(errors_.NumErrors() > 0); }
};

} // namespace

#endif