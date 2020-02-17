#ifndef __OPTIONS_H_
#define __OPTIONS_H_

#include "string"
#include "NamesList.h"

namespace SingNames {

enum CompilerMode {CM_SINGLE_PACKAGE, CM_TEST};
enum class FileSolveError {OK, AMBIGUOUS, NOT_FOUND};

class Options {
    NamesList   packages_src_dirs_;

    const char  *source_;
    const char  *out_file_;
    const char  *make_file_;
    const char  *c_format_source_;

    int             test_to_perform_;
    CompilerMode    mode_;
    bool            skip_usage_errors_;
    bool            debug_build_;
    bool            verbose_;

    // for the parser
    bool        waiting_a_value_;
    char        tag_;
    bool        error_;
    bool        must_print_help_;

    void SetOption(const char *value);
    void ParseSingleArg(const char *arg);
    void CheckArgumentsCombintion(void);
    void PrintHelp(void);
    void ReadFromFile(const char *filename);
    void PrependInclusionPath(string *full, const char *inclusion_path, const char *package_name);
public:
    bool ParseArgs(int argc, char *argv[]);
    int  GetTestMode(void) { return(test_to_perform_); }
    const char *GetSourceName(void) { return(source_); }
    const char *GetMakeFileDir(void) { return(make_file_); }
    const char *GetOutputFile(void) { return(out_file_); }
    CompilerMode GetMode(void) { return(mode_); }
    const char *GetSrcDir(int index) const { return(packages_src_dirs_.GetName(index)); }
    bool AreUsageErrorsEnabled(void) { return(!skip_usage_errors_); }

    // helper
    FileSolveError SolveFileName(FILE **fh, string *full, const char *partial);
};

} // namespace

#endif