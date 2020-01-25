#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "FileName.h"

namespace SingNames {

void Options::SetOption(const char *value)
{
    switch (tag_) {
    case 'I':
        packages_src_dirs_.AddName(value);
        break;
    case 'L':
        libraries_src_dirs_.AddName(value);
        break;
    case 'l':
        libraries_.AddName(value);
        break;
    case 'p':
        if (package_ != NULL) {
            printf("\nYou can specify '-p' only once\n");
            error_ = true;
        } else {
            package_ = value;
        }
        break;
    case 'd':
        if (package_dir_ != NULL) {
            printf("\nYou can specify '-d' only once\n");
            error_ = true;
        } else {
            package_dir_ = value;
        }
        break;
    case 't':
        if (tmp_dir_ != NULL) {
            printf("\nYou can specify '-t' only once\n");
            error_ = true;
        } else {
            tmp_dir_ = value;
        }
        break;
    case 'o':
        if (out_file_ != NULL) {
            printf("\nYou can specify '-o' only once\n");
            error_ = true;
        } else {
            out_file_ = value;
        }
        break;
    case 'f':
        if (c_format_source_ != NULL) {
            printf("\nYou can specify '-f' only once\n");
            error_ = true;
        } else {
            c_format_source_ = value;
        }
        break;
    case '^':
        test_to_perform_ = atoi(value);
        break;
    }
}

void Options::ParseSingleArg(const char *arg)
{
    if (gcc_) {
        if (strcmp(arg, "--gcc") == 0) {
            gcc_ = false;
        } else {
            gcc_options_.AddName(arg);
        }
    } else if (strcmp(arg, "-gcc") == 0) {
        gcc_ = true;
    } else if (arg[0] == '@') {
        ReadFromFile(arg + 1);
    } else if (waiting_a_value_) {
        SetOption(arg);
        waiting_a_value_ = false;
    } else {
        bool unknown_option = false;

        if (arg[0] != '-') {
            printf("\nOptions must start with -\n");
            error_ = true;
        } else if (strcmp(arg, "-clean") == 0) {
            clean_ = true;
        } else if (strlen(arg) != 2) {
            unknown_option = true;
        } else {
            switch (arg[1]) {
            case 'I':
            case 'L':
            case 'l':
            case 'p':
            case 'd':
            case 't':
            case 'o':
            case 'f':
            case '^':
                waiting_a_value_ = true;
                tag_ = arg[1];
                break;
            case 'v':
                verbose_ = true;
                break;
            case 'u':
                skip_usage_errors_ = true;
                break;
            case 'h':
            case '?':
                must_print_help_ = true;
                break;
            default:
                unknown_option = true;
                break;
            }
        }
        if (unknown_option) {
            printf("\nUnknown option ");
            printf(arg);
            printf("\n");
            error_ = true;
        }
    }
}

void Options::CheckArgumentsCombintion(void)
{
    if (test_to_perform_ != 0) {
        mode_ = CM_TEST;
    } else if (package_ != NULL) {
        mode_ = CM_SINGLE_PACKAGE;
        if (package_dir_ == NULL || tmp_dir_ == NULL) {
            printf("\nYou need to specify a package source directory and an output directory.\n");
            error_ = true;
        }
        if (libraries_src_dirs_.GetNamesCount() != 0 || libraries_.GetNamesCount() != 0 || 
            gcc_options_.GetNamesCount() != 0 || out_file_ != NULL) {
            printf("\nWarning: You entered some extra options which are not needed for this action (compiling a single package to c++)\n");
            printf("digit 'sing -h' for help\n");
        }
    } else {
        mode_ = CM_FULL_PROJECT;
        if (out_file_ == NULL || tmp_dir_ == NULL || package_dir_ == NULL) {
            printf("\nYou need to specify input and temporary directories, plus the output file.\n");
            error_ = true;
        }
    }
}

void Options::PrintHelp(void)
{
    printf(
        "\nTo compile a package to c++ :"
        "\nsing -p <package_name> -d <package_directory> -I <included packages search directory> -t <output directory>"
        "\nYou can specify multiple search directories. Also note the space between the option flag and the option value.\n"
        "\nTo build the project :"
        "\nsing -d <root sources directory> -I <included packages search directory> -L <included libraries directory>"
        "       -l <library name> -t <root directory for intermediate files> -o <output file>"
        "\nYou can specify multiple package/library search directories."
        "\nYou can include options for gcc if you place them between the flags -gcc ... --gcc\n"
        "\nOther options you can always use:"
        "\n-h or -?                         help"
        "\n-clean                           forces recompilation"
        "\n-v                               verbose"
        "\n-u                               skip usage test on declarations"      
        "\n-f <format_description_file>    to change the synthesized c++ format"
        "\n@<file>                         to read the options from <file>"
    );
}

void Options::ReadFromFile(const char *filename)
{
    FILE    *fd;
    int     ch;
    string  element;

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        printf("\nCan't open %s\n", filename);
        error_ = true;
        return;
    }
    while (!error_) {
        ch = getc(fd);
        if (ch == EOF) break;
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            if (element.length() > 0) {
                ParseSingleArg(element.c_str());
                element = "";
            }   // else ignore it !!
        } else if (ch == '\"' && element.length() == 0) {
            for (;;) {
                ch = getc(fd);
                if (ch == EOF || ch == '\"') break;
                if (ch != '\n' && ch != '\r') {
                    element += ch;
                }
            }
            if (element.length() > 0) {
                ParseSingleArg(element.c_str());
                element = "";
            }
        } else {
            element += ch;
        }
    }
    if (!error_ && element.length() > 0) {
        ParseSingleArg(element.c_str());
    }
    fclose(fd);
}

bool Options::ParseArgs(int argc, char *argv[])
{
    int     ii;
    bool    gccoption = false;

    // reset all
    package_ = NULL;
    package_dir_ = NULL;
    tmp_dir_ = NULL;
    out_file_ = NULL;
    c_format_source_ = NULL;

    clean_ = false;
    verbose_ = false;
    test_to_perform_ = 0;
    skip_usage_errors_ = false;

    gcc_ = false;
    waiting_a_value_ = false;
    error_ = false;
    must_print_help_ = false;

    // parse all the arguments
    for (ii = 1; ii < argc && !error_; ++ii) {
        ParseSingleArg(argv[ii]);
    }

    // final checks
    if (must_print_help_) {
        PrintHelp();
        return(false);
    }
    if (!error_) {
        if (waiting_a_value_) {
            printf("\nMissing the value of the last option\n");
            error_ = true;
        } else {
            CheckArgumentsCombintion();
        }
    }
    if (error_) {
        printf("digit 'sing -h' for help\n");
        return(false);
    }
    return(true);
}

FileSolveError Options::SolveFileName(FILE **fh, string *full, const char *partial)
{
    FILE    *fd;
    int     index = 0;
    string  fullname;

    // found ?
    fd = nullptr;
    while (fd == nullptr) {
        const char *search = GetSrcDir(index++);
        if (search == nullptr) {
            break;
        }
        PrependInclusionPath(&fullname, search, partial);
        fd = fopen(fullname.c_str(), "rb");
    }

    // ambiguous ?
    if (fd == nullptr) {
        return(FileSolveError::NOT_FOUND);
    } else {
        FILE *ft;

        if (full != nullptr) {
            *full = fullname;
        }
        while (true) {
            const char *search = GetSrcDir(index++);
            if (search == nullptr) {
                break;
            }
            PrependInclusionPath(&fullname, search, partial);
            ft = fopen(fullname.c_str(), "rb");
            if (ft != nullptr) {
                fclose(ft);
                fclose(fd);
                return(FileSolveError::AMBIGUOUS);
            }
        }
    }

    if (fh != nullptr) {
        *fh = fd;
    } else {
        fclose(fd);
    }
    return(FileSolveError::OK);
}

void Options::PrependInclusionPath(string *full, const char *inclusion_path, const char *package_name)
{
    *full = inclusion_path;
    int lastchar = full->c_str()[full->size() - 1];
    if (lastchar != '\\' && lastchar != '/') {
        (*full) += '/';
    }
    if (*package_name != '\\' && *package_name != '/') {
        *full += package_name;
    } else {
        *full += package_name + 1;
    }
    FileName::FixBackSlashes(full);
    FileName::ExtensionSet(full, "sing");
}

} // namespace
