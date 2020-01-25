//#include <stdlib.h>
//#include <complex>
#include "symbols_storage.h"

namespace SingNames {

IAstDeclarationNode *SymbolsStorage::FindDeclaration(const char *name)
{
    IAstDeclarationNode *node = FindGlobalDeclaration(name);
    if (node != NULL) return(node);
    return(FindLocalDeclaration(name));
}

IAstDeclarationNode *SymbolsStorage::FindGlobalDeclaration(const char *name)
{
    //auto search = globals_.find(name);
    //if (search != globals_.end()) {
    //    return(search->second);
    //}
    int position = globals_names_.LinearSearch(name);
    if (position != -1) {
        return(globals_nodes_[position]);
    }
    return(NULL);
}

IAstDeclarationNode *SymbolsStorage::FindLocalDeclaration(const char *name)
{
    int position = locals_names_.LinearSearch(name);
    if (position != -1) {
        return(locals_nodes_[position]);
    }
    return(NULL);
}

IAstDeclarationNode *SymbolsStorage::EnumerateInnerDeclarations(int idx)
{
    int base;
    int len = scopes_top_.size();
    int top = locals_nodes_.size();

    if (len == 0) {
        base = 0;
    } else {
        base = scopes_top_[len - 1];
    }
    if (base + idx >= top) {
        return(nullptr);
    }
    return(locals_nodes_[base + idx]);
}

IAstDeclarationNode *SymbolsStorage::EnumerateGlobalDeclarations(int idx)
{
    if (idx >= (int)globals_nodes_.size()) {
        return(nullptr);
    }
    return(globals_nodes_[idx]);
}

bool SymbolsStorage::InsertName(const char *name, IAstDeclarationNode *declaration)
{
    if (strlen(name) < 1) {
        return(false);
    }

    // search in globals and locals (the latter is empty if we have not opened a scope)
    auto search = FindDeclaration(name);
    if (search != NULL) {
        return(false);
    }

    // insert locally if a (loacal) scope is open
    if (scopes_top_.size() != 0) {
        locals_nodes_.push_back(declaration);
        locals_names_.AddName(name);
    } else {
        globals_nodes_.push_back(declaration);
        globals_names_.AddName(name);
        //globals_[name] = declaration;
    }
    return(true);
}

void SymbolsStorage::OpenScope(void)
{
    scopes_top_.push_back(locals_nodes_.size());
}

void SymbolsStorage::CloseScope(void)
{
    if (scopes_top_.size() <= 1) {
        locals_names_.Reset();
        locals_nodes_.clear();
        scopes_top_.clear();
    } else {
        int cut_from = scopes_top_[scopes_top_.size() - 1];
        int cut_to = locals_nodes_.size();
        scopes_top_.pop_back();
        locals_names_.Erase(cut_from, cut_to);
        locals_nodes_.erase(cut_from, cut_to);
    }
}

} // namespace