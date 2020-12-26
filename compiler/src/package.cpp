#include "package.h"
#include "Parser.h"
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
    FileName::FixBackSlashes(&fullpath_);
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

bool Package::advanceTo(PkgStatus wanted_status)
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
        parser.Init(&lexer);
        root_ = parser.ParseAll(&errors_, wanted_status != PkgStatus::FULL);

        checked_ = false;
        status_ = wanted_status;
    }
    return(errors_.NumErrors() == 0);
}

bool Package::check(AstChecker *checker)
{
    if (status_ != PkgStatus::FOR_REFERENCIES && status_ != PkgStatus::FULL ||
        errors_.NumErrors() > 0) {
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

void Package::applyPatch(int start, int stop, const char *new_text)
{   
    source_.patch(start, stop, new_text);
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

} // namespace