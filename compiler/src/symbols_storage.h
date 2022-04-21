#ifndef SYMBOLS_STORAGE_H
#define SYMBOLS_STORAGE_H

#include <unordered_map>
#include "ast_nodes.h"

namespace std {
    template<> struct hash<SingNames::string>
    {
        typedef std::size_t result_type;
        result_type operator()(SingNames::string const& str) const noexcept
        {
            std::hash<std::string> stdhash;
            return(stdhash(str.c_str()));
        }
    };
}

namespace SingNames {

class SymbolsStorage {
    NamesList                                           globals_names_;
    vector<IAstDeclarationNode*>                        globals_nodes_;

    NamesList                                           locals_names_;
    vector<IAstDeclarationNode*>                        locals_nodes_;
    vector<int>                                         scopes_top_;
public:
    void ClearAll(void);
    bool InsertName(const char *name, IAstDeclarationNode *declaration);    // returns false if the name is duplicated
    void OpenScope(void);
    void CloseScope(void);
    IAstDeclarationNode *FindDeclaration(const char *name);
    IAstDeclarationNode *FindGlobalDeclaration(const char *name);
    IAstDeclarationNode *FindLocalDeclaration(const char *name);

    IAstDeclarationNode *EnumerateInnerDeclarations(int idx);
    IAstDeclarationNode *EnumerateGlobalDeclarations(int idx);
};

} // namespace

#endif