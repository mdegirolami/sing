#include "package.h"
#include "FileName.h"
#include "ast_checks.h"

namespace SingNames {

Package::Package()
{
    root_ = nullptr;
    status_ = PkgStatus::UNLOADED;
    checked_ = false;
}

Package::~Package()
{
    if (root_ != nullptr) delete root_;
}

void Package::Init(const char *filename, int32_t idx)
{
    clearParsedData();
    fullpath_ = filename;
    status_ = PkgStatus::UNLOADED;
    idx_ = idx;
}

void Package::clearParsedData(void)
{
    if (status_ == PkgStatus::FOR_REFERENCIES || status_ == PkgStatus::FULL) {
        status_ = PkgStatus::LOADED;
    }
    errors_.Reset();
    if (root_ != nullptr) {
        delete root_;
        root_ = nullptr;
    }
    symbols_.ClearAll();
}

bool Package::Load()
{
    FILE    *fd;

    // already there
    if (status_ != PkgStatus::UNLOADED) {
        return(status_ != PkgStatus::ERROR);
    }

    // reset all. prepare for a new load
    if (root_ != nullptr) delete root_;
    root_ = nullptr;
    errors_.Reset();
    status_ = PkgStatus::ERROR;     // in case we early-exit

    // check here and don't emit an error message
    if (fullpath_.length() < 1) {
        return(false);
    }

    fd = fopen(fullpath_.c_str(), "rb");

    if (fd == nullptr) {
        errors_.AddError("Can't open file", -1, -1, -1, -1);
        return(false);
    }

    // read the file length (is it too much ?)
    fseek(fd, 0, SEEK_END);
    int len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    if (len > max_filesize) {
        fclose(fd);
        errors_.AddError("File too big (exceeds 10 Mbytes)", -1, -1, -1, -1);
        return(false);
    }

    // read the content into the string
    char *buffer = source_.getBufferForLoad(len);
    int read_result = fread(buffer, len, 1, fd);
    fclose(fd);
    if (read_result != 1 && len != 0) {
        errors_.AddError("Error reading the file", -1, -1, -1, -1);
        return(false);
    }
    status_ = PkgStatus::LOADED;
    return(true);
}

bool Package::advanceTo(PkgStatus wanted_status, bool for_intellisense)
{
    // nonsense
    if (wanted_status == PkgStatus::UNLOADED || wanted_status == PkgStatus::ERROR || status_ == PkgStatus::ERROR) {
        return(false);
    }

    // need to load ?
    if (status_ == PkgStatus::UNLOADED) {
        Load();
    }
    if (status_ == PkgStatus::ERROR) {
        return(false);
    } 

    // done ? note: here status_ is LOADED or more advanced
    if (wanted_status == PkgStatus::LOADED) {
        return(true);
    }

    // do we need to revert ?
    if (status_ == PkgStatus::FOR_REFERENCIES && wanted_status == PkgStatus::FULL) {
        clearParsedData();
    }

    //
    // at this point the wanted_status can only be FULL or FOR_REF, status can be only:
    // FOR_REF, FULL, LOADED - this last is the only case in which we need to take action.
    // 
    if (status_ == PkgStatus::LOADED) {
        Lexer   lexer;
        Parser  parser;

        // parse    
        lexer.Init(source_.getAsString());
        parser.Init(&lexer, nullptr, idx_);
        ParseMode mode = ParseMode::FULL;
        if (wanted_status != PkgStatus::FULL) {
            mode = ParseMode::FOR_REFERENCE;
        } else if (for_intellisense) {
            mode = ParseMode::INTELLISENSE;
        }
        root_ = parser.ParseAll(&errors_, mode);

        checked_ = false;
        status_ = wanted_status;
    }
    return(errors_.NumErrors() == 0);
}

bool Package::check(AstChecker *checker)
{
    if (status_ != PkgStatus::FOR_REFERENCIES && status_ != PkgStatus::FULL) {
        return(false);
    }
    if (checked_) return(true);
    checked_ = true;
    if (!checker->CheckAll(root_, &errors_, &symbols_, status_ == PkgStatus::FULL)) {
        SortErrors();
        return(false);
    }
    return(true);
}

IAstDeclarationNode *Package::findSymbol(const char *name, bool *is_private)
{
    IAstDeclarationNode *node = symbols_.FindDeclaration(name);
    if (node != nullptr) {
        *is_private = !node->IsPublic();
    } else if (root_ != nullptr && root_->private_symbols_.LinearSearch(name) >= 0) {
        *is_private = true;
    } else {
        *is_private = false;
    }
    return(node);
}

const char *Package::GetErrorString(int index) const
{
    const char *message;
    int         row, col;
    static char fullmessage[4096];

    message = errors_.GetError(index, &row, &col);
    if (message == nullptr) return(nullptr);
    if (row >= 0 && col >= 0) {
        sprintf(fullmessage, "%d:%d: %s", row, col, message);
    } else {
        return(message);
    }
    return(fullmessage);
}

const char *Package::GetError(int index, int *row, int *col, int *endrow, int *endcol) const
{
    return(errors_.GetError(index, row, col, endrow, endcol));
}

void Package::applyPatch(int from_row, int from_col, int to_row, int to_col, int allocate, const char *newtext)
{   
    source_.patch(from_row, from_col, to_row, to_col, allocate, newtext);
    clearParsedData();
}

void Package::insertInSrc(const char *newtext)
{
    source_.insert(newtext);
    clearParsedData();
}

bool Package::depends_from(int index)
{
    if (root_ != nullptr) {
        int count = root_->dependencies_.size();
        for (int ii = 0; ii < count; ++ii) {
            AstDependency *dep = root_->dependencies_[ii];
            if (dep->package_index_ == index && dep->GetUsage() != DependencyUsage::UNUSED) {
                return(true);
            }
        }
    }
    return(false);
}

void Package::parseForSuggestions(CompletionHint *hint)
{
    Lexer   lexer;
    Parser  parser;
    string  row;

    // convert row/col from vsc to sing conventions
    source_.GetLine(&row, hint->row);
    hint->col = source_.VsCol2Offset(row.c_str(), hint->col);
    hint->col = source_.offset2SingCol(row.c_str(), hint->col);
    hint->row++;

    clearParsedData();
    if (status_ != PkgStatus::LOADED) {
        return;
    }
    lexer.Init(source_.getAsString());
    parser.Init(&lexer, hint, idx_);
    root_ = parser.ParseAll(&errors_, ParseMode::INTELLISENSE);
    checked_ = false;
    status_ = PkgStatus::FULL;
}

void Package::getAllPublicTypeNames(NamesList *names)
{
    if (root_ == nullptr) return;
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_TYPE && declaration->IsPublic()) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            names->AddName(tdecl->name_.c_str());
        }
    }
}

void Package::getAllPublicDeclNames(NamesList *names)
{
    if (root_ == nullptr) return;
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->IsPublic()) {
            if (declaration->GetType() == ANT_VAR) {
                VarDeclaration *decl = (VarDeclaration*)declaration;
                names->AddName(decl->name_.c_str());
            } else if (declaration->GetType() == ANT_FUNC) {
                FuncDeclaration *decl = (FuncDeclaration*)declaration;
                names->AddName(decl->name_.c_str());
            } else if (declaration->GetType() == ANT_TYPE) {
                TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
                names->AddName(tdecl->name_.c_str());
            }
        }
    }
}

IAstDeclarationNode *Package::getDeclaration(const char *name)
{
    symbols_.FindGlobalDeclaration(name);
}

bool Package::GetPartialPath(string *path, int row, int col)
{
    if (row < 0 || col < 8) return(false);
    source_.GetLine(path, row);
    if (path->length() < 9) {
        return(false);
    } 

    // past stuff after the trigger and the trigger itself
    int off = source_.VsCol2Offset(path->c_str(), col);
    path->erase(off);
    
    const char *src = path->data();

    // skip blanks
    while (*src == ' ' || *src == '\t') {
        ++src;
    }

    // requires ?
    if (strncmp(src, "requires", 8) != 0) {
        return(false);
    }

    // skip blanks to the opening "
    src += 8;
    while (*src == ' ' || *src == '\t') {
        ++src;
    }

    // skip '"' - if present !
    if (*src == '"') ++src;

    // keep the final part
    path->erase(0, src - path->data());
    return(true);
}

int Package::SearchFunctionStart(int *row, int *col)
{
    string line;    
    int level = 0;
    int position= 0;

    for (int rr = *row; rr >= 0; --rr) {
        source_.GetLine(&line, rr);
        int scan;
        if (rr == *row) {
            scan = source_.VsCol2Offset(line.c_str(), *col);
        } else {
            scan = line.length() - 1;
        }
        for (; scan >= 0; --scan) {
            switch (line[scan]) {
            case '(':
                if (level == 0) {
                    *row = rr;
                    *col = source_.offset2VsCol(line.c_str(), scan);
                    return(position);
                } else {
                    --level;
                }
                break;
            case ')':
                ++level;
                break;
            case ',':
                if (level == 0) ++position;
                break;
            case ';':
            case '}': 
            case '{':
                return(-1);
                break; 
            }
        }
    }
    return(-1);
}

void Package::GetSymbolAt(string *symbol, int *dot_row, int *dot_col, int row, int col)
{
    string line;   

    // defaults
    *symbol = "";
    *dot_row = -1;
    *dot_col = -1;

    // find the symbol beginning
    source_.GetLine(&line, row);    
    int scan = source_.VsCol2Offset(line.c_str(), col);
    while (scan > 0 && IsSymbolCharacter(line[scan])) --scan;
    if (isdigit(line[scan])) return;
    int begin = scan + 1;    

    // is it preceeded by a '.' operator ?
    while (scan > 0 && isblank(line[scan])) --scan;
    if (line[scan] == '.') {
        *dot_col = source_.offset2VsCol(line.c_str(), scan);
        *dot_row = row;
    }

    // read the symbol
    scan = begin;
    while (IsSymbolCharacter(line[scan])) {
        *symbol += line[scan++];
    }
}

IAstNode *Package::getDeclarationReferredAt(const char *symbol, int row, int col)
{
    if (root_ == nullptr) return(nullptr);
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_VAR) {
            VarDeclaration *decl = (VarDeclaration*)declaration;
            if (decl->name_ == symbol) {
                return(decl);
            }
        } else if (declaration->GetType() == ANT_FUNC) {
            FuncDeclaration *decl = (FuncDeclaration*)declaration;
            if (decl->name_ == symbol && !decl->is_class_member_) {
                return(decl);
            }
            AstBlock *block = decl->block_;
            if (block != nullptr && block->GetPositionRecord()->Includes(row + 1, col)) {
                AstFuncType *func = decl->function_type_;
                if (func != nullptr) {
                    for (int jj = 0; jj < func->arguments_.size(); ++jj) {
                        if (func->arguments_[jj]->name_ == symbol) {
                            return(func->arguments_[jj]);
                        }
                    }
                }
                IAstNode *node = getDeclarationInBlockBefore(block, symbol, row, col);
                if (node != nullptr) return(node);
            }
        } else if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *decl = (TypeDeclaration*)declaration;
            if (decl->name_ == symbol) {
                return(decl);
            }
        }
    }
    return(nullptr);
}

IAstNode *Package::getDeclarationInBlockBefore(AstBlock *block, const char *symbol, int row, int col)
{
    bool done = false;

    if (block == nullptr || !block->GetPositionRecord()->Includes(row + 1, col)) return(nullptr);
    vector<IAstNode*> *items = &block->block_items_;
    for (int ii = 0; ii < items->size() && !done; ++ii) {
        IAstNode *node = (*items)[ii];
        if (node == nullptr || node->GetPositionRecord()->start_row > row) {
            return(nullptr);
        }
        switch (node->GetType()) {
        case ANT_VAR:
            if (strcmp(((VarDeclaration*)node)->name_.c_str(), symbol) == 0) {
                return(node);
            }
            break;
        case ANT_BLOCK:
            {
                IAstNode *ret = getDeclarationInBlockBefore((AstBlock*)node, symbol, row, col);
                if (ret != nullptr) return(ret);
            }
            break;
        case ANT_WHILE:
            {
                IAstNode *ret = getDeclarationInBlockBefore(((AstWhile*)node)->block_, symbol, row, col);
                if (ret != nullptr) return(ret);
            }
            break;
        case ANT_IF:
            {
                AstIf *ifdesc = (AstIf*)node;
                for (int jj = 0; jj < ifdesc->blocks_.size(); ++jj) {
                    IAstNode *ret = getDeclarationInBlockBefore(ifdesc->blocks_[jj], symbol, row, col);
                    if (ret != nullptr) return(ret);
                }
            }
            break;
        case ANT_FOR:
            {
                AstFor *fordesc = (AstFor*)node;
                AstBlock *block = fordesc->block_;
                if (block != nullptr && block->GetPositionRecord()->Includes(row + 1, col)) {
                    if (fordesc->index_ != nullptr && fordesc->index_->name_ == symbol) {
                        return(fordesc->index_);
                    }
                    if (fordesc->iterator_ != nullptr && fordesc->iterator_->name_ == symbol) {
                        return(fordesc->iterator_);
                    }
                    IAstNode *ret = getDeclarationInBlockBefore(fordesc->block_, symbol, row, col);
                    if (ret != nullptr) return(ret);
                }
            }
            break;
        case ANT_SWITCH:
            {
                AstSwitch *switchdesc = (AstSwitch*)node;
                for (int jj = 0; jj < switchdesc->statements_.size(); ++jj) {
                    if (switchdesc->statements_[jj]->GetType() == ANT_BLOCK) {
                        IAstNode *ret = getDeclarationInBlockBefore((AstBlock*)switchdesc->statements_[jj], symbol, row, col);
                        if (ret != nullptr) return(ret);
                    }
                }
            }
            break;
        case ANT_TYPESWITCH:
            if (node->GetPositionRecord()->Includes(row + 1, col)) {
                AstTypeSwitch *switchdesc = (AstTypeSwitch*)node;
                if (switchdesc->reference_ != nullptr && switchdesc->reference_->name_ == symbol) {
                    return(switchdesc->reference_);
                }
                for (int jj = 0; jj < switchdesc->case_statements_.size(); ++jj) {
                    if (switchdesc->case_statements_[jj]->GetType() == ANT_BLOCK) {
                        IAstNode *ret = getDeclarationInBlockBefore((AstBlock*)switchdesc->case_statements_[jj], symbol, row, col);
                        if (ret != nullptr) return(ret);
                    }
                }
            }
            break;
        }
    }
    return(nullptr);
}

bool Package::ConvertPosition(PositionInfo *pos, int *row, int *col)
{
    if (pos->start_row < 1) return(false);
    *row = pos->start_row - 1;

    string line;   
    source_.GetLine(&line, *row);    
    int off = source_.SingCol2offset(line.c_str(), pos->start_col);
    *col = source_.offset2VsCol(line.c_str(), off);
    return(true);
}

bool Package::IsSymbolCharacter(char value)
{
    return(value >= 'a' && value <= 'z' || 
           value >= 'A' && value <= 'Z' || 
           value == '_' || 
           value >= '0' && value <= '9');
}

FuncDeclaration *Package::findFuncDefinition(AstClassType *classtype, const char *symbol)
{
    string classname;

    if (root_ == nullptr) return(nullptr);
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *tdecl = (TypeDeclaration*)declaration;
            if (tdecl->type_spec_ == classtype) {
                classname = tdecl->name_;
                break;
            }
        }
    }
    if (classname == "") {
        return(nullptr);
    }
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetType() == ANT_FUNC) {
            FuncDeclaration *decl = (FuncDeclaration*)declaration;
            if (decl->is_class_member_ && decl->name_ == symbol && decl->classname_ == classname) {
                return(decl);
            }
        }
    }
    return(nullptr);
}

bool Package::SymbolIsInMemberDeclaration(AstClassType **classnode, IAstDeclarationNode **membernode, 
                                            const char *symbol, int row, int col)
{
    if (root_ == nullptr) return(false);
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        if (declaration->GetPositionRecord()->start_row > row + 1) {
            break;
        }
        if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *decl = (TypeDeclaration*)declaration;
            if (decl->type_spec_ != nullptr && decl->GetPositionRecord()->Includes(row + 1, col)) {
                if (decl->type_spec_->GetType() == ANT_CLASS_TYPE) {
                    *classnode = (AstClassType*)decl->type_spec_;
                    *membernode = (*classnode)->getMemberDeclaration(symbol);
                    if (*membernode != nullptr && (*membernode)->GetPositionRecord()->start_row == row + 1) {
                        return(true);
                    }
                } else if (decl->type_spec_->GetType() == ANT_INTERFACE_TYPE) {
                    *membernode = ((AstInterfaceType*)decl->type_spec_)->getMemberDeclaration(symbol);
                    if (*membernode != nullptr && (*membernode)->GetPositionRecord()->start_row == row + 1) {
                        *classnode = nullptr;
                        *membernode = nullptr;
                        return(true);
                    };
                }
            }
        }
    }
    return(false);    
}

void Package::getAllPublicNames(vector<SymbolNfo> *vv)
{
    SymbolNfo   nfo;

    if (root_ == nullptr) return;
    for (int ii = 0; ii < (int)root_->declarations_.size(); ++ii) {
        IAstDeclarationNode *declaration = root_->declarations_[ii];
        bool isgood = false;
        if (declaration->GetType() == ANT_VAR) {
            VarDeclaration *decl = (VarDeclaration*)declaration;
            nfo.name = decl->name_;
            nfo.type = decl->HasOneOfFlags(VF_READONLY) ? SymbolType::cvar : SymbolType::var;
            isgood = true;
        } else if (declaration->GetType() == ANT_FUNC) {
            FuncDeclaration *decl = (FuncDeclaration*)declaration;
            if (decl->is_class_member_) {
                nfo.name = decl->classname_ + '.' + decl->name_;
                nfo.type =  SymbolType::method;
            } else {
                nfo.name = decl->name_;
                nfo.type =  SymbolType::fun;
            }
            isgood = true;
        } else if (declaration->GetType() == ANT_TYPE) {
            TypeDeclaration *decl = (TypeDeclaration*)declaration;
            nfo.name = decl->name_;
            nfo.type = SymbolType::type;
            if (decl->type_spec_ != nullptr) {
                switch (decl->type_spec_->GetType()) {
                case ANT_CLASS_TYPE:
                    nfo.type = SymbolType::ctype;
                    break;
                case ANT_ENUM_TYPE:
                    nfo.type = SymbolType::etype;
                    break;
                case ANT_INTERFACE_TYPE:
                    nfo.type = SymbolType::itype;
                    break;
                }
            }
            isgood = true;            
        }
        if (isgood) {
            PositionInfo *pos = declaration->GetPositionRecord();
            nfo.row = pos->start_row - 1;
            nfo.col = pos->start_col;
            vv->push_back(nfo);
        }
    }
}

} // namespace