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
    if (root_ != nullptr) delete root_;
    root_ = nullptr;
    errors_.Reset();
    fullpath_ = filename;
    FileName::FixBackSlashes(&fullpath_);
    status_ = PkgStatus::UNLOADED;
}

bool Package::Load(PkgStatus wanted_status)
{
    Lexer   lexer;
    Parser  parser;
    FILE    *fd;

    // already there
    if (status_ >= wanted_status) {
        return(true);
    }

    // already trayed or request is nonsense
    if (status_ == PkgStatus::ERROR || wanted_status <= PkgStatus::ERROR) {
        return(false);
    }

    // reset all. prepare for a new load
    if (root_ != nullptr) delete root_;
    root_ = nullptr;
    errors_.Reset();
    status_ = PkgStatus::ERROR;     // in case we early-exit
    checked_ = false;

    fd = fopen(fullpath_.c_str(), "rb");

    if (fd == nullptr) {
        errors_.AddError("Can't open file", -1, -1);
        return(false);
    }

    // parse    
    lexer.Init(fd);
    parser.Init(&lexer);
    root_ = parser.ParseAll(&errors_, wanted_status < PkgStatus::FULL);
    lexer.CloseFile();

    // examine result
    if (root_ == NULL) {
        return(false);
    }
    status_ = wanted_status;
    return(true);
}

bool Package::check(AstChecker *checker)
{
    if (status_ == PkgStatus::ERROR) return(false);
    if (checked_) return(true);
    checked_ = true;
    if (!checker->CheckAll(root_, &errors_, &symbols_, status_ == PkgStatus::FULL)) {
        status_ = PkgStatus::ERROR;
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

const char *Package::GetError(int index) const
{
    const char *message;
    int         row, col;
    static char fullmessage[1024];

    message = errors_.GetError(index, &row, &col);
    if (message == nullptr) return(nullptr);
    if (row >= 0 && col >= 0) {
        sprintf(fullmessage, "%d:%d: %s", row, col, message);
    } else {
        return(message);
    }
    return(fullmessage);
}

} // namespace