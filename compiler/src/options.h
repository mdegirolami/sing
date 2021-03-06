#ifndef __OPTIONS_H_
#define __OPTIONS_H_

#include "string"
#include "NamesList.h"

namespace SingNames {

enum class FileSolveError {OK, AMBIGUOUS, NOT_FOUND};

class Options {
    NamesList   packages_src_dirs_;

    const char  *source_;
    const char  *out_file_;
    const char  *c_format_source_;

    int             test_to_perform_;
    bool            skip_usage_errors_;
    bool            debug_build_;
    bool            verbose_;
    bool            create_d_file_;
    bool            generate_h_only_;
    bool            server_mode_;

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
    const char *GetOutputFile(void) { return(out_file_); }
    const char *GetSrcDir(int index) const { return(packages_src_dirs_.GetName(index)); }
    bool AreUsageErrorsEnabled(void) { return(!skip_usage_errors_); }
    bool MustCreateDFile(void) { return(create_d_file_); }
    bool GenerateHOnly(void) { return(generate_h_only_); }
    bool ServerMode(void) { return(server_mode_); }
    void GetAllFilesIn(NamesList *names, const char *path);

    // helper
    FileSolveError SolveFileName(FILE **fh, string *full, const char *partial);
};

} // namespace

#endif