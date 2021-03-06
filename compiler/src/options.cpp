#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "FileName.h"
#include "sio.h"

namespace SingNames {

void Options::SetOption(const char *value)
{
    switch (tag_) {
    case 0:
        if (source_ != nullptr) {
            printf("\nYou can specify only one source (first is %s, second is %s)\n", source_, value);
            error_ = true;
        } else {
            source_ = value;
        }
        break;
    case 'I':
        packages_src_dirs_.AddName(value);
        break;
    case 'o':
        if (out_file_ != nullptr) {
            printf("\nYou can specify '-o' only once\n");
            error_ = true;
        } else {
            out_file_ = value;
        }
        break;
    case 'f':
        if (c_format_source_ != nullptr) {
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
    if (arg[0] == '@') {
        ReadFromFile(arg + 1);
    } else if (waiting_a_value_) {
        SetOption(arg);
        waiting_a_value_ = false;
    } else {
        bool unknown_option = false;

        if (arg[0] != '-') {
            tag_ = 0;
            SetOption(arg);
        } else if (strcmp(arg, "-MF") == 0) {
            create_d_file_ = true;
        } else if (strlen(arg) != 2) {
            unknown_option = true;
        } else {
            switch (arg[1]) {
            case 'o':
            case 'f':
            case '^':
            case 'I':
                waiting_a_value_ = true;
                tag_ = arg[1];
                break;
            case 'g':
                debug_build_ = true;
                break;
            case 'v':
                verbose_ = true;
                break;
            case 'u':
                skip_usage_errors_ = true;
                break;
            case 'p':
                generate_h_only_ = true;
                break;
            case 's':
                server_mode_ = true;
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
            printf("\nUnknown option %s\n", arg);
            error_ = true;
        }
    }
}

void Options::CheckArgumentsCombintion(void)
{
    if (test_to_perform_ == 0 && !server_mode_) {
        if (source_ == nullptr || out_file_ == nullptr) {
            printf("\nYou at least need to specify a source and an output file.\n");
            error_ = true;
        }
    }
}

void Options::PrintHelp(void)
{
    printf(
        "\nUsage : sing [options] <source_file_name>"
        "\n"
        "\nOptions:"
        "\n -h or -?    help"
        "\n -f <file>   output format options. To change the synthesized c++ format"
        "\n -g          debug build"
        "\n -I <dir>    included sing search directory. You can specify multiple search directories."
        "\n -MF         generate makefile with dependencies"
        "\n -o <file>   output filename (3 files are generated with extensions: .h, .cpp and .sm)"
        "\n -u          skip usage test on declarations"      
        "\n -v          verbose"
        "\n -p          generate header only"
        "\n @<file>      to read command line arguments from <file>"
        "\n"
        "\nPlease note the space between the option flag and the option value !!\n\n"
    );
}

void Options::ReadFromFile(const char *filename)
{
    FILE    *fd;
    int     ch;
    string  element;

    fd = fopen(filename, "rb");
    if (fd == nullptr) {
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
    source_ = nullptr;
    out_file_ = nullptr;
    c_format_source_ = nullptr;

    verbose_ = false;
    test_to_perform_ = 0;
    skip_usage_errors_ = false;
    debug_build_ = false;
    create_d_file_ = false;
    generate_h_only_ = false;
    server_mode_ = false;

    waiting_a_value_ = false;
    error_ = false;
    must_print_help_ = false;

    // parse all the arguments
    for (ii = 1; ii < argc && !error_; ++ii) {
        if (argv[ii][0] == '<' || argv[ii][1] == '>') continue;
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
        printf("please digit 'sing -h' for help\n\n");
        return(false);
    }
    if (c_format_source_ != nullptr) {
        if (!synth_options_.load(c_format_source_)) {
            printf("cannot read the %s format file\n\n", c_format_source_);
        }
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
            FileName::Normalize(full);
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

void Options::GetAllFilesIn(NamesList *names, const char *path)
{
    std::string fullname, drive, pathbody, base, extension;
    std::vector<std::string> nn;
    std::vector<sing::FileInfo> info;
    
    int index = 0;
    while (true) {
        const char *search = GetSrcDir(index++);
        if (search == nullptr) {
            return;
        }
        std::string fullname = search;
        fullname += "/";
        fullname += path;
        fullname = sing::pathFix(fullname.c_str());
        sing::dirRead(fullname.c_str(), sing::DirFilter::all, &nn, &info);
        for (int ii = 0; ii < nn.size(); ++ii) {
            sing::pathSplit(nn[ii].c_str(), &drive, &pathbody, &base, &extension);
            //const char *src = nn[ii].c_str() + fullname.length();
            //if (*src == '/') ++src;
            if (info[ii].is_dir_) {
                names->AddName(base.c_str());
            } else if (extension == "sing" || extension == "SING") {
                names->AddName(base.c_str());
            }
        }
    }
}

} // namespace
