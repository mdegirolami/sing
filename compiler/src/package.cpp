#include "package.h"
#include "Parser.h"
#include "FileName.h"
#include "ast_checks.h"

namespace SingNames {

Package::Package()
{
    root_ = nullptr;
    status_ = PkgStatus::UNLOADED;
    gap_ = 0;
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
        errors_.AddError("Can't open file", -1, -1);
        return(false);
    }

    // read the file length (is it too much ?)
    fseek(fd, 0, SEEK_END);
    int len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    if (len > 10*1024*1024) {
        fclose(fd);
        errors_.AddError("File too big (exceeds 10 Mbytes)", -1, -1);
        return(false);
    }

    // read the content into the string
    source_[0].clear();
    source_[1].clear();
    gap_ = 0;
    source_[0].reserve(len + 1);
    source_[0].resize(len);

    int read_result = fread(&source_[0][0], len, 1, fd);
    fclose(fd);
    if (read_result != 1) {
        errors_.AddError("Error reading the file", -1, -1);
        source_[0].clear();
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

    // load
    if (status_ == PkgStatus::UNLOADED) {
        Load();
    }
    if (status_ == PkgStatus::ERROR) {
        return(false);
    } 

    if (status_ == PkgStatus::FOR_REFERENCIES && wanted_status == PkgStatus::FULL) {
        clearParsedData();
    }

    //
    // at this point the status can be only:
    // FOR_REF (if target is FOR_REF or LOADED)
    // FULL
    // LOADED - this is the only case in which we need to take action.
    // 
    if (status_ == PkgStatus::LOADED && wanted_status != PkgStatus::LOADED) {
        Lexer   lexer;
        Parser  parser;

        // collapse source_[0] and source_[1] in a single vector
        int tocopy = source_[1].size() - gap_;
        if (tocopy > 0) {
            source_[0].reserve(source_[0].size() + tocopy + 1);
            source_[0].insert_range(source_[0].size(), tocopy, &source_[1][gap_]);
            source_[1].clear();
        }

        // terminate
        source_[0].push_back(0);

        // parse    
        lexer.Init(&source_[0][0]);
        parser.Init(&lexer);
        root_ = parser.ParseAll(&errors_, wanted_status != PkgStatus::FULL);

        // unterminate
        source_[0].pop_back();

        checked_ = false;
    }
    status_ = wanted_status;
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

void Package::applyPatch(int start, int stop, const char *new_text)
{   
    // rearrange data in source_ vectors to have 
    // everything before insertion point in source_[0]
    // everything past insertion point in source_[1]
    if (start < source_[0].size()) {
        if (stop < source_[0].size()) {

            // copy the stuff past stop (the insertion point) in source_[1]
            int tomove = source_[0].size() - stop;

            // if tomove can't fit gap_, grow the gap
            int togrow = tomove - gap_;
            if (togrow > 0) {

                // leave 1024 bytes of extra space to prevent/limit further expansions
                int prevlen = source_[1].size();
                int newlen = prevlen +  togrow + 1024;
                source_[1].resize(newlen);

                char *src = &source_[1][prevlen - 1];
                char *dst = &source_[1][newlen - 1];
                for (int ii = prevlen - gap_; ii > 0; --ii) {
                    *dst-- = *src--;
                }

                gap_ += togrow + 1024;
            }

            // move into gap_
            char *src = &source_[0][stop];
            char *dst = &source_[1][gap_ - tomove];
            for (; tomove > 0; --tomove) {
                *dst++ = *src++;
            }
        } else {

            // delete from source_[1] all before the stop/insertion point (virtually, just change gap_)
            int stop_in_v1 = stop - source_[0].size() + gap_;
            gap_ = stop_in_v1; 
        }

        // clean up all past start/inserion point from source_[0]
        source_[0].erase(start, source_[0].size());
    } else {

        int stop_in_v1 = stop - source_[0].size() + gap_;

        // copy everything before start/insertion in source_[0]
        source_[0].insert_range(source_[0].size(), start - source_[0].size(), &source_[1][gap_]);
        gap_ = stop_in_v1;
    }

    // append past the insertion point    
    source_[0].insert_range(source_[0].size(), strlen(new_text), new_text);
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