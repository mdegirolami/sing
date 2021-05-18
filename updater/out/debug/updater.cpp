#include "updater.h"
#include "sio.h"
#include "sys.h"
#include "str.h"
#include "buildupd.h"

static void collectSources(const char *root, std::vector<Source> *buffer);
static bool copyFolderFromSdk(const char *sdk, const char *root, const char *folder);
static bool areThereDuplications(std::string *path1, std::string *path2, const std::vector<Source> &all_srcs, sing::map<std::string, int32_t> *base2idx);
static std::string getDefaultTarget(const char *root);
static void replace_PHOLD(const char *name, const char *default_target, bool forwin, bool debug);

int32_t updater(const std::vector<std::string> &argv)
{
    // check args
    if (argv.size() < 3) {
        sing::print("\r\nUsage: updater <project_path> <sdk path>\r\n");
        return (1);
    }

    // take out '/' from the end of the string, if present. Normalize.
    const std::string root = sing::pathFix(argv.at(1).c_str());
    const std::string sdk = sing::pathFix(argv.at(2).c_str());
    const std::string default_target = getDefaultTarget(root.c_str());

    // collect sources
    std::vector<Source> sources;
    collectSources(root.c_str(), &sources);

    // if there are no sources, init with hello_world
    if (sources.size() < 1) {
        if (!copyFolderFromSdk(sdk.c_str(), root.c_str(), "sing") || !copyFolderFromSdk(sdk.c_str(), root.c_str(), "src")) {
            sing::print("Error: Can't copy sources to the project directory !!");
            return (1);
        }
        collectSources(root.c_str(), &sources);
    }

    // if the build directory doesn't exist, let's create one
    sing::FileInfo nfo;
    if (sing::fileGetInfo((root + "/build").c_str(), &nfo) != 0 || !nfo.is_dir_) {
        if (!copyFolderFromSdk(sdk.c_str(), root.c_str(), "build")) {
            sing::print("Error: Can't copy build files to the project directory !!");
            return (1);
        }
    }

    // update sdk_location.ninja
    const std::string sdkfile = root + "/build/sdk_location.ninja";
    bool sdkfile_ok = false;
    std::string sdkfile_content;
    if (sing::fileReadText((root + "/build/sdk_location.ninja").c_str(), &sdkfile_content) == 0) {
        std::string discard;
        if (sing::split(sdkfile_content.c_str(), "=", &discard, &sdkfile_content)) {
            sing::cutLeadingSpaces(&sdkfile_content);
            sing::splitAny(sdkfile_content.c_str(), " \t\r\n", &sdkfile_content, &discard);
            if (sing::fileGetInfo(sdkfile_content.c_str(), &nfo) == 0 && nfo.is_dir_) {
                sdkfile_ok = true;
            }
        }
    }
    if (!sdkfile_ok) {
        sdkfile_content = sing::s_format("%s%s%s", "sdk = ", sdk.c_str(), "\r\n");
        if (sing::fileWriteText(sdkfile.c_str(), sdkfile_content.c_str()) != 0) {
            sing::print("Error: Can't write sdk_location.ninja !!");
            return (1);
        }
    }

    // build index and check duplications
    std::string path1;
    std::string path2;
    sing::map<std::string, int32_t> srcbase2idx;
    if (areThereDuplications(&path1, &path2, sources, &srcbase2idx)) {
        sing::print(sing::s_format("%s%s%s%s", "Error: Base filenames must be unique !! Conflict between ", path1.c_str(), " and ", path2.c_str()).c_str());
        return (1);
    }

    // update build_aux.ninja
    bool has_mods = false;
    const std::string result = fixBuild(&has_mods, (root + "/build/build_aux.ninja").c_str(), sources, srcbase2idx);
    if (result != "") {
        sing::print(("Error: /build/build_aux.ninja, " + result).c_str());
        return (1);
    }

    // set the default target name into build files (replacing "<PHOLD>")
    replace_PHOLD((root + "/build/build_linux_debug.ninja").c_str(), default_target.c_str(), false, true);
    replace_PHOLD((root + "/build/build_linux_release.ninja").c_str(), default_target.c_str(), false, false);
    replace_PHOLD((root + "/build/build_win_debug.ninja").c_str(), default_target.c_str(), true, true);
    replace_PHOLD((root + "/build/build_win_release.ninja").c_str(), default_target.c_str(), true, false);

    // sources changed. clean up temporary files
    if (has_mods) {
        sing::dirRemove((root + "/out").c_str(), true);
        sing::dirRemove((root + "/build/obj_d").c_str(), true);
        sing::dirRemove((root + "/build/obj_r").c_str(), true);
    }

    // create required directories
    sing::dirCreate((root + "/build/obj_d").c_str());
    sing::dirCreate((root + "/build/obj_r").c_str());
    sing::dirCreate((root + "/bin/linux").c_str());
    sing::dirCreate((root + "/bin/win").c_str());

    // create temporary directories for sing generated c files
    sing::map<std::string, int32_t> paths;
    for(auto &src : sources) {
        if (src.ext_ == "sing") {
            paths.insert(src.path_.c_str(), 0);
        }
    }
    for(int32_t idx = 0, idx__top = paths.size(); idx < idx__top; ++idx) {
        sing::dirCreate(sing::s_format("%s%s%s", root.c_str(), "/out/debug/", paths.key_at(idx).c_str()).c_str());
        sing::dirCreate(sing::s_format("%s%s%s", root.c_str(), "/out/release/", paths.key_at(idx).c_str()).c_str());
    }

    // create and fill .vscode if not existent
    if (sing::fileGetInfo((root + "/.vscode").c_str(), &nfo) != 0 || !nfo.is_dir_) {
        if (!copyFolderFromSdk(sdk.c_str(), root.c_str(), ".vscode")) {
            sing::print("Error: Can't copy .vscode files to the project directory !!");
            return (1);
        }
    }

    // fix .vscode/launch.json
    std::string to_launch = default_target + "_d";
    std::string original;
    if (sing::fileReadText((root + "/.vscode/launch.json").c_str(), &original) == 0) {
        std::string buffer = original;
        const sing::OsId id = sing::getOs();
        switch (id) {
        case sing::OsId::linux: 
            {
                sing::replaceAll(&buffer, "/win", "/linux");
                sing::replaceAll(&buffer, "/osx", "/linux");
                sing::replace(&buffer, ".exe", "");
            }
            break;
        case sing::OsId::osx: 
            {
                sing::replaceAll(&buffer, "/win", "/osx");
                sing::replaceAll(&buffer, "/linux", "/osx");
                sing::replace(&buffer, ".exe", "");
            }
            break;
        case sing::OsId::win: 
            {
                sing::replaceAll(&buffer, "/linux", "/win");
                sing::replaceAll(&buffer, "/osx", "/win");
                sing::Range range;
                if (sing::find(buffer.c_str(), to_launch.c_str(), &range)) {
                    int32_t end_pos = 0;
                    if (sing::compareAt(buffer.c_str(), range.end_, "\"", &end_pos) == 0) {
                        sing::insert(&buffer, range.end_, ".exe");
                    }
                }
                to_launch += ".exe";
            }
            break;
        }
        sing::replace(&buffer, "<PHOLD>", to_launch.c_str());
        if (buffer != original) {
            if (sing::fileWriteText((root + "/.vscode/launch.json").c_str(), buffer.c_str()) != 0) {
                sing::print("Error: Can't update .vscode/launch.json !!");
                return (1);
            }
        }
    }

    // create sing_sense.txt if it doesn't exist
    if (sing::fileGetInfo((root + "/.vscode/sing_sense.txt").c_str(), &nfo) != 0) {
        const int64_t err = sing::fileWriteText((root + "/.vscode/sing_sense.txt").c_str(), "sing\r\n");
        if (err != 0) {
            sing::print("Error: Can't write sing_sense.txt");
            return (1);
        }
    }

    // create .gitignore if it doesn't exist
    if (sing::fileGetInfo((root + "/.gitignore").c_str(), &nfo) != 0) {
        const int64_t err = sing::fileWriteText((root + "/.gitignore").c_str(), "bin/*\r\n"
            "build/obj*\r\n"
            "out/*\r\n");
        if (err != 0) {
            sing::print("Error: Can't write .gitignore");
            return (1);
        }
    }

    sing::print("\r\nAll done...");
    return (0);
}

static void collectSources(const char *root, std::vector<Source> *buffer)
{
    std::vector<std::string> tmp;

    const std::string singroot = sing::s_format("%s%s", root, "/sing");
    sing::dirReadNames(singroot.c_str(), sing::DirFilter::regular, &tmp, true);
    for(auto &name : tmp) {
        if (sing::hasSuffix(name.c_str(), ".sing", true)) {
            sing::cutPrefix(&name, singroot.c_str());
            sing::cutPrefix(&name, "/");
            Source src;
            std::string drive;
            sing::pathSplit(name.c_str(), &drive, &src.path_, &src.base_, &src.ext_);
            (*buffer).push_back(src);
        }
    }

    const std::string cpproot = sing::s_format("%s%s", root, "/src/");
    tmp.clear();
    sing::dirReadNames(cpproot.c_str(), sing::DirFilter::regular, &tmp, true);
    for(auto &name : tmp) {
        if (sing::hasSuffix(name.c_str(), ".cpp", true)) {
            sing::cutPrefix(&name, cpproot.c_str());
            sing::cutPrefix(&name, "/");
            Source src;
            std::string drive;
            sing::pathSplit(name.c_str(), &drive, &src.path_, &src.base_, &src.ext_);
            (*buffer).push_back(src);
        }
    }
}

static bool copyFolderFromSdk(const char *sdk, const char *root, const char *folder)
{
    // source and dst directories
    const std::string source = sing::s_format("%s%s%s", sdk, "/template/", folder);
    const std::string dst = sing::s_format("%s%s%s", root, "/", folder);

    // read the files list
    std::vector<std::string> tocopy;
    sing::dirReadNames(source.c_str(), sing::DirFilter::regular, &tocopy, false);

    // be sure the destination directory exists
    if (sing::dirCreate(dst.c_str()) != 0) {
        return (false);
    }

    // for every file in source dir..
    std::string dstfile;
    std::vector<uint8_t> buffer;
    for(auto &srcfile : tocopy) {
        dstfile = srcfile;
        sing::cutPrefix(&dstfile, source.c_str(), false);
        buffer.clear();
        if (sing::fileRead(srcfile.c_str(), &buffer) != 0) {
            return (false);
        }
        if (sing::fileWrite((dst + dstfile).c_str(), buffer) != 0) {
            return (false);
        }
    }

    return (true);
}

static bool areThereDuplications(std::string *path1, std::string *path2, const std::vector<Source> &all_srcs, sing::map<std::string, int32_t> *base2idx)
{
    int64_t count = -1;
    for(auto &src : all_srcs) {
        ++count;
        if ((*base2idx).has(src.base_.c_str())) {
            *path1 = all_srcs.at((*base2idx).get(src.base_.c_str())).getFullName();
            *path2 = src.getFullName();
            return (true);
        }
        (*base2idx).insert(src.base_.c_str(), (int32_t)count);
    }
    return (false);
}

static std::string getDefaultTarget(const char *root)
{
    std::string last;
    std::string discard;
    if (!sing::rsplitAny(root, "/\\", &discard, &last)) {
        return (root);
    }
    return (last);
}

static void replace_PHOLD(const char *name, const char *default_target, bool forwin, bool debug)
{
    std::string buffer;
    if (sing::fileReadText(name, &buffer) == 0) {
        std::string target = default_target;
        if (debug) {
            target += "_d";
        }
        if (forwin) {
            target += ".exe";
        }
        if (sing::replace(&buffer, "<PHOLD>", target.c_str()) != sing::npos) {
            sing::fileWriteText(name, buffer.c_str());
        }
    }
}
