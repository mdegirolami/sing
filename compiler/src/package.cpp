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

void Package::Init(const char *filename)
{
    clearParsedData();
    fullpath_ = filename;
    status_ = PkgStatus::UNLOADED;
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
    if (read_result != 1) {
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
        parser.Init(&lexer, nullptr);
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
    } else if (root_->private_symbols_.LinearSearch(name) >= 0) {
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
    Lexer           lexer;
    Parser          parser;

    clearParsedData();
    if (status_ != PkgStatus::LOADED) {
        return;
    }
    lexer.Init(source_.getAsString());
    parser.Init(&lexer, hint);
    root_ = parser.ParseAll(&errors_, ParseMode::INTELLISENSE);
    checked_ = false;
    status_ = PkgStatus::FULL;
}

void Package::getAllPublicTypeNames(NamesList *names)
{
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
    path->erase(col);
    
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
        for (int scan = rr == *row ? *col : line.length() - 1; scan >= 0; --scan) {
            switch (line[scan]) {
            case '(':
                if (level == 0) {
                    *row = rr;
                    *col = scan;
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

} // namespace