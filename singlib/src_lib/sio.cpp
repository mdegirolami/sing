#include "sio.h"
#include "str.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cinttypes>

#ifdef _WIN32    
#include <windows.h>
#include <direct.h>
#include <conio.h>
#else
#include <termios.h>
#include <dirent.h>
#include <unistd.h>
#endif

// NOTE:
// to be added to the automatically generated File class declaration: 
/*
    void    *fd_;
    int     dir_;
    File();
*/

namespace sing {

// values for dir_
static const int dir_read = 1;    
static const int dir_write = 2;    
static const int dir_seek = 3;    

static const int MAX_SING_PATH = 1024;     // some bigger than standard

void pathCleanParts(std::vector<std::string> &parts, bool absolute);
void addBlanks(std::string *dst, int32_t count);
void addChars(std::string *dst, char cc, int32_t count);
void synthFormat(char *dst, int32_t flags, const char *base_conversion, int size, int fract = -1);
std::string formatFloatExp(const double val, int32_t exp_mult, const int32_t field_len, const int32_t fract_digits, const int32_t flags);

// implemented in str.cpp
#ifdef _WIN32
std::string utf16_to_8(const wchar_t *src);
void utf8_to_16(const char *src, std::vector<wchar_t> *dst);
Error dirReadCore_r(WIN32_FIND_DATAW *desc, wchar_t *buffer, DirFilter filter, 
                    std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive);
Error dirRemove_r(WIN32_FIND_DATAW *desc, wchar_t *buffer);
#else
Error dirReadCore_r(char *buffer, DirFilter filter, 
                    std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive);
Error dirRemove_r(char *buffer);
#endif

inline void dirswitch(FILE *fd, int *olddir, int dir) 
{ 
    if (dir != *olddir) {
        if (*olddir != dir_seek && fd != nullptr) {
            int64_t pos = ftell(fd);
            if (pos != -1) fseek(fd, pos, SEEK_SET);
        }
        *olddir = dir;
    }
} 

FileInfo::FileInfo()
{
    last_modification_time_ = 0;
    length_ = 0;
    is_dir_ = false;
}

char File::id__;

File::File()
{
    fd_ = nullptr;
}

File::~File()
{
    close();
}

Error File::open(const char *name, const char *mode)
{
    close();

    dir_ = dir_seek;        
    if (mode[0] != 'r' && mode[0] != 'w' || mode[1] != 0 && mode[1] != '+' || strlen(mode) > 2) {
        return(-1);
    }
    char mm[5];
    strcpy(mm, mode);
    strcat(mm, "b");
#ifdef _WIN32
    std::vector<wchar_t> wname;
    std::vector<wchar_t> wmm;
    utf8_to_16(name, &wname);
    utf8_to_16(mm, &wmm);
    fd_ = _wfopen(wname.data(), wmm.data());
#else
    fd_ = fopen(name, mm);
#endif
    if (fd_ == nullptr) {
        return(-1);
    }
    return(0);
}

Error File::close()
{
    if (fd_ != nullptr) {
        if (fclose((FILE*)fd_) == EOF) {
            fd_ = nullptr;
            return(-1);
        }
    }
    fd_ = nullptr;
    return(0);
}

Error File::flush()
{
    if (fd_ == nullptr) return(-1);
    if (fflush((FILE*)fd_) == EOF) {
        return(-1);
    }
    return(0);
}

Error File::getInfo(FileInfo *nfo)
{
    struct stat buf;
    if (fstat(fileno((FILE*)fd_), &buf) == -1) {
        return(errno);
    }
    nfo->length_ = buf.st_size; 
    nfo->last_modification_time_ = buf.st_mtime;
    nfo->is_dir_ = false;
    return(0);
}

Error File::get(uint8_t *value)
{    
    dirswitch((FILE*)fd_, &dir_, dir_read); 
    int cc = getc((FILE*)fd_);
    if (cc == EOF) {
        return(-1);
    }
    *value = cc;
    return(0);
}

Error File::gets(const int64_t maxbytes, std::string *value)
{
    dirswitch((FILE*)fd_, &dir_, dir_read); 
    int cc = 0;
    *value = "";
    do {
        cc = getc((FILE*)fd_);
        if (cc == EOF) {
            if (feof((FILE*)fd_)) {
                return(0);
            } else {
                return(-1);
            }
        }
        if (cc != 0) *value += cc;
    } while (cc != '\n' && cc != 0 && (int64_t)value->length() < maxbytes);
    size_t len = value->length();
    if (len > 1 && value->at(len - 1) == '\n' && value->at(len - 2) == '\r') {
        value->resize(len - 2);
        *value += '\n';
    }
    return(0);
}

Error File::read(const int64_t count, std::vector<uint8_t> *dst, const bool append)
{
    size_t pos;

    if (count < 0) return(-1);
    dirswitch((FILE*)fd_, &dir_, dir_read); 
    if (append) {
        pos = dst->size();
        dst->resize(pos + (size_t)count);
    } else {
        pos = 0;
        dst->clear();
        dst->resize(count);
    }
    if (fread(&(*dst)[pos], count, 1, (FILE*)fd_) == 1) {
        return(0);
    }
    return(-1);
}

Error File::put(const uint8_t value)
{
    dirswitch((FILE*)fd_, &dir_, dir_write); 
    if (putc(value, (FILE*)fd_) != EOF) {
        return(0);
    }
    return(-1);
}

Error File::puts(const char *value)
{
    dirswitch((FILE*)fd_, &dir_, dir_write); 
    if (fputs(value, (FILE*)fd_) != EOF) {
        return(0);
    }
    return(-1);   
}

Error File::write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from)
{
    if (from < 0 || fd_ == nullptr) return(-1);
    dirswitch((FILE*)fd_, &dir_, dir_write); 
    int64_t towrite = std::min(count, (int64_t)src.size() - from);
    if (towrite > 0) {
        if (fwrite(&src[0] + from, towrite, 1, (FILE*)fd_) != 1) {
            return(-1);
       }
    }
    return(0);
}

Error File::seek(const int64_t pos, SeekMode mode)
{
    // must match the sequence in which SeekMode members are declared
    int whence[] = {SEEK_SET, SEEK_CUR, SEEK_END}; 
    dir_ = dir_seek;
    return(fseek((FILE*)fd_, pos, whence[(int)mode]));
}

Error File::tell(int64_t *pos)
{
    int64_t pp = ftell((FILE*)fd_);
    if (pp == -1) return(errno);
    *pos = pp;
    return(0);
}

bool File::eof() const
{
    return(feof((FILE*)fd_));
}

Error fileRemove(const char *filename)
{    
#ifdef _WIN32
    std::vector<wchar_t> wname;
    utf8_to_16(filename, &wname);
    return(_wremove(wname.data()));
#else
    return(remove(filename));
#endif
}

Error fileRename(const char *old_name, const char *new_name)
{
#ifdef _WIN32
    std::vector<wchar_t> wold;
    std::vector<wchar_t> wnew;
    utf8_to_16(old_name, &wold);
    utf8_to_16(new_name, &wnew);
    return(_wrename(wold.data(), wnew.data()));
#else
    return(rename(old_name, new_name));
#endif
}

Error fileGetInfo(const char *filename, FileInfo *nfo)
{
#ifdef _WIN32
    struct _stat buf;
    std::vector<wchar_t> wfilename;
    utf8_to_16(filename, &wfilename);
    if (_wstat(wfilename.data(), &buf) == -1) {
        return(errno);
    }
    nfo->length_ = buf.st_size; 
    nfo->last_modification_time_ = buf.st_mtime;
#else
    struct stat buf;
    if (stat(filename, &buf) == -1) {
        return(errno);
    }
    nfo->length_ = buf.st_size; 
    nfo->last_modification_time_ = buf.st_mtime;
#endif
    return(0);    
}

Error fileRead(const char *filename, std::vector<uint8_t> *dst)
{
    File        ff;
    FileInfo    nfo;
    Error       err;

    err = ff.open(filename, "r");
    if (err != 0) return(err);
    err = ff.getInfo(&nfo);
    if (err != 0) return(err);
    err = ff.read(nfo.length_, dst, true);
    if (err != 0) return(err);
    return(ff.close());
}

Error fileReadText(const char *filename, std::string *dst)
{
    File        ff;
    FileInfo    nfo;
    Error       err;

    err = ff.open(filename, "r");
    if (err != 0) return(err);
    err = ff.getInfo(&nfo);
    if (err != 0) return(err);
    dst->reserve(nfo.length_);
    err = ff.gets(nfo.length_ + 1, dst);
    if (err != 0) return(err);
    return(ff.close());
}

Error fileWrite(const char *filename, const std::vector<uint8_t> &src)
{
    File        ff;
    Error       err;

    err = ff.open(filename, "w");
    if (err != 0) return(err);
    err = ff.write(src.size(), src);
    if (err != 0) return(err);
    return(ff.close());
}

Error fileWriteText(const char *filename, const char *src)
{
    File        ff;
    Error       err;

    err = ff.open(filename, "w");
    if (err != 0) return(err);
    err = ff.puts(src);
    if (err != 0) return(err);
    return(ff.close());
}

Error setCwd(const char *directory)
{
#ifdef _WIN32
    std::vector<wchar_t> wdirectory;
    utf8_to_16(directory, &wdirectory);
    return(_wchdir(wdirectory.data()));
#else
    return(chdir(directory));
#endif
}

std::string getCwd()
{
#ifdef _WIN32
    wchar_t name[MAX_SING_PATH + 1];
    if (_wgetcwd(name, MAX_SING_PATH) == nullptr) {
        return(".");
    }
    return(utf16_to_8(name));
#else
    char name[PATH_MAX + 1];
    if (getcwd(name, PATH_MAX + 1) == nullptr) {
        return(".");
    }
    return(name);
#endif
}

void pathSplit(const char *fullname, std::string *drive, std::string *path, std::string *base, std::string *extension)
{
    int32_t dd = pathGetDriveIdx(fullname);
    if (dd != -1) {
        *drive = 'a' + dd;
        *drive += ':';
        fullname += 2;
    } else {
        *drive = "";
    }
    if (!rsplitAny(fullname, "/\\", path, base, SplitMode::sm_separator_left)) {
        *base = fullname;
        *path = "";
    }
    if (!rsplitAny(base->c_str(), ".", base, extension, SplitMode::sm_drop)) {
        *extension = "";
    }
}

std::string pathJoin(const char *drive, const char *path, const char *base, const char *extension)
{
    std::string full;
    full.reserve(strlen(drive) + strlen(path) + strlen(base) + strlen(extension) + 3);
    full = drive;
    if (!hasSuffix(full.c_str(), ":")) {
        full += ':';
    }
    full += path;
    if (!hasSuffix(full.c_str(), "/") && !hasSuffix(full.c_str(), "\\")) {
        full += '/';
    }
    full += base;
    int num_pts = 0;
    if (hasSuffix(full.c_str(), ".")) num_pts++;
    if (hasPrefix(extension, ".")) num_pts++;
    if (num_pts == 0) {
        full += '.';
    } else if (num_pts == 2) {
        ++extension;
    }
    full += extension;
    return(full);
}

void pathSplitAll(const char *fullname, int32_t *drive, bool *absolute, std::vector<std::string> *parts)
{
    std::string temp, part;

    *drive = pathGetDriveIdx(fullname);
    *absolute = pathIsAbsolute(fullname);
    if (*drive != -1) fullname += 2;
    if (*absolute) fullname += 1;
    temp = fullname;
    parts->clear();    
    while (splitAny(temp.c_str(), "/\\", &part, &temp, SplitMode::sm_drop)) {
        if (part != "") {
            parts->push_back(part);
        }
    }
    if (temp != "") {
        parts->push_back(temp);
    }
}

std::string pathJoinAll(const int32_t drive, const bool absolute, const std::vector<std::string> &parts)
{
    std::string result;

    // determine output string length
    int len = 0;
    if (drive != -1) len += 2;
    if (absolute) ++len;
    for (int src = 0; src < parts.size(); ++src) {
        len += parts[src].length() + 1;
    }    
    result.reserve(len);

    // compose
    result = "";
    if (drive != -1) {
        result += 'a' + drive;
        result += ':';
    }
    if (absolute) result += '/';
    int top = (int)parts.size() - 1;
    for (int ii = 0; ii < top; ++ii) {
        result += parts[ii];
        result += '/';
    }
    if (!parts.empty()) {
        result += parts.back();
    }
    return(result);
}

std::string pathFix(const char *path)
{
    int32_t                     drive;
    bool                        absolute;   
    std::vector<std::string>    parts;

    pathSplitAll(path, &drive, &absolute, &parts);
    pathCleanParts(parts, absolute);
    return(pathJoinAll(drive, absolute, parts));
}

void pathSetExtension(std::string *path, const char *ext)
{
    for (int ii = path->length() - 1; ii > 0; --ii) {
        char cc = (*path)[ii];
        if (cc == '.') {
            path->resize(ii);
            break;
        } else if (cc == '/' || cc == '\\') {
            break;
        }
    }
    if (*ext != '.') *path += '.';
    *path += ext;
}

bool pathToAbsolute(std::string *result, const char *relpath, const char *basepath)
{
    std::string base;

    // get actual base (in case of default parm) and check absolute/rel. paths
    if (basepath == nullptr || basepath[0] == 0) {
        base = getCwd();
        basepath = base.c_str();
    }

    // checks !
    if (pathIsAbsolute(relpath) || !pathIsAbsolute(basepath)) {
        return(false);
    }
    int32_t d0 = pathGetDriveIdx(relpath);
    int32_t d1 = pathGetDriveIdx(basepath);
    if (d0 != -1 && d1 != -1 && d0 != d1) {
        return(false);
    }   
    int32_t drive = d0 == -1 ? d1 : d0; 
    if (d0 != -1) relpath += 2;
    if (d1 != -1) basepath += 2;

    // add all the pieces
    *result = "";
    if (drive != -1) {
        *result = 'a' + drive;
        *result += ':';
    }
    *result += basepath;
    *result += '/';
    *result += relpath;
    *result = pathFix(result->c_str());
    return(true);
}

bool pathToRelative(std::string *result, const char *abspath, const char *basepath)
{
    bool    absolute, base_absolute;
    int32_t drive, base_drive;
    std::vector<std::string> parts;
    std::vector<std::string> base_parts;

    // get actual base (in case of default parm) and check absolute/rel. paths
    // and split to parts
    if (basepath == nullptr || basepath[0] == 0) {
        pathSplitAll(getCwd().c_str(), &base_drive, &base_absolute, &base_parts);
    } else {
        pathSplitAll(basepath, &base_drive, &base_absolute, &base_parts);
    }

    // split abspath
    pathSplitAll(abspath, &drive, &absolute, &parts);

    // checks !
    if (!base_absolute || !absolute || drive != -1 && base_drive != -1 && drive != base_drive) {
        return(false);
    }

    // this is needed to make them comparable
    pathCleanParts(parts, true);
    pathCleanParts(base_parts, true);

    // discard the common part
    int common;
    int len = (int)std::min(base_parts.size(), parts.size());
    for (common = 0; common < len && parts[common] == base_parts[common]; ++common);

    // navigate upward if required
    int depth = base_parts.size() - common;
    base_parts.clear(); // recycle
    while (depth-- > 0) {
        base_parts.push_back("..");
    }

    // then move to the desired place
    for (int src = common; src < parts.size(); ++src) {
        base_parts.push_back(parts[src]);
    }

    *result = pathJoinAll(-1, false, base_parts);

    return(true);
}

bool pathIsAbsolute(const char *path)
{
    const char *tocheck;

    if (path[0] == 0) return(false);
    if (path[1] == ':') {
        tocheck = path + 2;
    } else {
        tocheck = path;
    }
    return(*tocheck == '/' || *tocheck == '\\');
}

int32_t pathGetDriveIdx(const char *path)
{
    if (path[0] != 0 && path[1] == ':') {
        if (path[0] >= 'a' && path[0] <= 'z') {
            return(path[0] - 'a');
        } else if (path[0] >= 'A' && path[0] <= 'Z') {
            return(path[0] - 'A');
        }
    }
    return(-1);
}

void pathCleanParts(std::vector<std::string> &parts, bool absolute)
{
    int dst = 0;
    for (int src = 0; src < parts.size(); ++src) {
        if (parts[src] == "..") {
            if (dst > 0) {
                if (parts[dst-1] == "..") {
                    parts[dst++] = parts[src];
                } else {
                    --dst;
                }
            } else {
                if (absolute) {
                    dst = 0;
                } else {
                    parts[dst++] = parts[src];
                }
            }            
        } else if (parts[src] != ".") {
            parts[dst++] = parts[src];
        }
    }
    parts.resize(dst);
}

#ifdef _WIN32

// returns 100 nS units since Jan 1st 1601
uint64_t packFILETIME(FILETIME *time)
{
    // join low/high part in a single value
    uint64_t wintime = ((uint64_t)time->dwHighDateTime << 32) + time->dwLowDateTime;    

    // convert to seconds since since 00:00, Jan 1 1970 UTC (Posix time)
    return(wintime/10000000 - 11644473600);
}

Error dirRead(const char *directory, DirFilter filter, std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive)
{
    WIN32_FIND_DATAW     desc;   // pretty big structure and there is no reason to have an item per recursion.
    std::vector<wchar_t> buffer;

    info->clear();
    names->clear();
    buffer.reserve(MAX_SING_PATH + 1);
    utf8_to_16(directory, &buffer);
    return(dirReadCore_r(&desc, buffer.data(), filter, names, info, recursive));
}

Error dirReadNames(const char *directory, DirFilter filter, std::vector<std::string> *names, bool recursive)
{
    WIN32_FIND_DATAW     desc;   // pretty big structure and there is no reason to have an item per recursion.
    std::vector<wchar_t> buffer;

    names->clear();
    buffer.reserve(MAX_SING_PATH + 1);
    utf8_to_16(directory, &buffer);
    return(dirReadCore_r(&desc, buffer.data(), filter, names, nullptr, recursive));
}

// buffer on entry has the directory path but is also used as a temp storage
Error dirReadCore_r(WIN32_FIND_DATAW *desc, wchar_t *buffer, DirFilter filter, 
                    std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive)
{
	HANDLE          search_handle;
    Error           err = 0;

    if (wcslen(buffer) + 4 > MAX_SING_PATH) {
        return(-1);
    }
    size_t clip = wcslen(buffer);
    wcscpy(buffer + clip, L"/*.*");
	search_handle = FindFirstFileW(buffer, desc);
	if (search_handle == INVALID_HANDLE_VALUE) return(-1);
    do {
        if ((desc->dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) == 0 && 
            wcscmp(desc->cFileName, L".") != 0 && wcscmp(desc->cFileName, L"..") != 0 && clip + 1 + wcslen(desc->cFileName) <= MAX_SING_PATH) {

            wcscpy(buffer + clip + 1, desc->cFileName);

            bool isdir = (desc->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (!(filter == DirFilter::regular && isdir || filter == DirFilter::directory && !isdir)) {
                names->push_back(utf16_to_8(buffer));
                if (info != nullptr) {
                    FileInfo nfo;
                    nfo.last_modification_time_ = packFILETIME(&desc->ftLastWriteTime);
                    nfo.length_ = desc->nFileSizeHigh * MAXDWORD + desc->nFileSizeLow;
                    nfo.is_dir_ = isdir;
                    info->push_back(nfo);
                }
            }
            if (isdir && recursive) {
                if (dirReadCore_r(desc, buffer, filter, names, info, recursive) != 0) {
                    err = -1;
                }
            }
        }
    } while(FindNextFileW(search_handle, desc) != 0);
    // err = GetLastError(); // note: 18 is "ERROR_NO_MORE_FILES"
    FindClose(search_handle);

    // restore buffer 
    buffer[clip] = 0;
    return(err);
}

Error dirRemove(const char *directory, const bool if_not_empty)
{
    WIN32_FIND_DATAW     desc;   // pretty big structure and there is no reason to have an item per recursion.
    std::vector<wchar_t> buffer;

    buffer.reserve(MAX_SING_PATH + 1);
    utf8_to_16(directory, &buffer);
    if (_wrmdir(buffer.data()) == 0) return(0);
    if (!if_not_empty) return(-1);
    return(dirRemove_r(&desc, buffer.data()));
}

Error dirRemove_r(WIN32_FIND_DATAW *desc, wchar_t *buffer)
{
	HANDLE          search_handle;
    Error           err = 0;

    size_t clip = wcslen(buffer) + 1;

    if (clip + 3 > MAX_SING_PATH) {
        return(-1);
    }
    wcscat(buffer, L"/*.*");
	search_handle = FindFirstFileW(buffer, desc);
	if (search_handle == INVALID_HANDLE_VALUE) return(-1);
    do {
        if (wcscmp(desc->cFileName, L".") != 0 && wcscmp(desc->cFileName, L"..") != 0) {
            if (wcslen(buffer) + wcslen(desc->cFileName) + 1 > MAX_SING_PATH) {
                err = -1;
            } else {
                wcscpy(buffer + clip, desc->cFileName);
                bool isdir = (desc->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                if (!isdir) {            
                    if (_wremove(buffer) != 0) {
                        err = -1;
                    }
                } else {
                    if (dirRemove_r(desc, buffer) != 0) {
                        err = -1;
                    }
                }
            }
        }
    } while(err == 0 && FindNextFileW(search_handle, desc) != 0);
    // err = GetLastError(); // note: 18 is "ERROR_NO_MORE_FILES"
    FindClose(search_handle);

    // restore buffer 
    buffer[clip - 1] = 0;

    if (err == 0) {
        if (_wrmdir(buffer) != 0) {
            err = -1;
        }
    }
    return(err);
}

Error dirCreate(const char *directory)
{
    int  top, scan;
    std::vector<wchar_t> buffer;

    utf8_to_16(directory, &buffer);
    wchar_t *pathcopy = &buffer[0];

    // strip trailing separator
    top = (int)wcslen(pathcopy);
    if (top < 1) return(-1);
    if (pathcopy[top-1] == '/' || pathcopy[top - 1] == '\\') {
        --top;
        pathcopy[top] = 0;
    }
    scan = top;

    // shorten the path 'till you find the first directory to be created 
    while (true) {
        if (_wmkdir(pathcopy) == 0) break;
        if (errno == EEXIST) break;

        // we have more than one component non existing, skip the last one
        for (--scan; scan > 0 && !(pathcopy[scan] == '/' || pathcopy[scan] == '\\'); --scan);
        if (scan == 0) {
            return(-1);
        }
        pathcopy[scan] = 0;
    } 

    // now create the other directories
    while (scan < top) {
        pathcopy[scan] = '/';
        scan = (int)wcslen(pathcopy);
        if (_wmkdir(pathcopy) != 0) {
            return(-1);
        }
    }
    return(0);
}

#else

Error dirRead(const char *directory, DirFilter filter, std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive)
{
    char buffer[MAX_SING_PATH + 1];

    if (strlen(directory) > MAX_SING_PATH) {
        return(-1);
    }
    info->clear();
    names->clear();
    strcpy(buffer, directory);
    return(dirReadCore_r(buffer, filter, names, info, recursive));
}

Error dirReadNames(const char *directory, DirFilter filter, std::vector<std::string> *names, bool recursive)
{
    char buffer[MAX_SING_PATH + 1];

    if (strlen(directory) > MAX_SING_PATH) {
        return(-1);
    }
    names->clear();
    strcpy(buffer, directory);
    return(dirReadCore_r(buffer, filter, names, nullptr, recursive));
}

// buffer on entry has the directory path but is also used as a temp storage
Error dirReadCore_r(char *buffer, DirFilter filter, 
                    std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive)
{
    Error           err = 0;
    struct dirent   *direntp;
    struct stat     stat_buf;

    // checks - must have at least space for \ and a single char named file
    size_t clip = strlen(buffer);
    if (strlen(buffer) + 2 > MAX_SING_PATH) {
        return(-1);
    }

    DIR *dir = opendir(buffer);
    if (dir == nullptr) return(-1);

    strcpy(buffer + clip, "/");
    while ((direntp = readdir(dir)) != nullptr) {
        if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0 && clip + 1 + strlen(direntp->d_name) <= MAX_SING_PATH) {
            strcpy(buffer + clip + 1, direntp->d_name);
            if (stat(buffer, &stat_buf) == -1) {
                err = -1;
                break;
            }
            bool regular = S_ISREG(stat_buf.st_mode);
            bool isdir = S_ISDIR(stat_buf.st_mode);
            if (!(filter == DirFilter::regular && !regular || filter == DirFilter::directory && !isdir)) {
                names->push_back(buffer);
                if (info != nullptr) {
                    FileInfo nfo;
                    nfo.length_ = stat_buf.st_size; 
                    nfo.last_modification_time_ = stat_buf.st_mtime;
                    nfo.is_dir_ = isdir;
                    info->push_back(nfo);
                }
            }
            if (isdir && recursive) {
                if (dirReadCore_r(buffer, filter, names, info, recursive) != 0) {
                    err = -1;
                }
            }
        }
    }
    closedir(dir);

    // restore buffer 
    buffer[clip] = 0;
    return(err);
}

Error dirRemove(const char *directory, const bool if_not_empty)
{
    char buffer[MAX_SING_PATH + 1];

    if (rmdir(directory) == 0) return(0);
    if (!if_not_empty) return(-1);
    if (strlen(directory) > MAX_SING_PATH) {
        return(-1);
    }
    strcpy(buffer, directory);
    return(dirRemove_r(buffer));
}

Error dirRemove_r(char *buffer)
{
    Error           err = 0;
    struct dirent   *direntp;
    struct stat     stat_buf;

    // checks - must have at least space for \ and a single char named file
    size_t clip = strlen(buffer);
    if (strlen(buffer) + 2 > MAX_SING_PATH) {
        return(-1);
    }

    DIR *dir = opendir(buffer);
    if (dir == nullptr) return(-1);

    strcpy(buffer + clip, "/");
    while (err == 0 && (direntp = readdir(dir)) != nullptr) {
        if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0) {
            if (clip + 1 + strlen(direntp->d_name) > MAX_SING_PATH) {
                err = -1;
            } else {
                strcpy(buffer + clip + 1, direntp->d_name);
                if (stat(buffer, &stat_buf) == -1) {
                    err = -1;
                    break;
                }
                bool isdir = S_ISDIR(stat_buf.st_mode);
                if (!isdir) {            
                    if (remove(buffer) != 0) {
                        err = -1;
                    }
                } else {
                    if (dirRemove_r(buffer) != 0) {
                        err = -1;
                    }
                }
            }
        }
    }
    closedir(dir);

    // restore buffer 
    buffer[clip] = 0;

    // delete the (now) empty directory
    if (err == 0) {
        if (rmdir(buffer) != 0) {
            err = -1;
        }
    }
    return(err);
}

Error dirCreate(const char *directory)
{
    int  top, scan;
    char pathcopy[MAX_SING_PATH + 1];

    if (strlen(directory) > MAX_SING_PATH) {
        return(-1);
    }
    strcpy(pathcopy, directory);

    // strip trailing separator
    top = (int)strlen(pathcopy);
    if (top < 1) return(-1);
    if (pathcopy[top-1] == '/' || pathcopy[top - 1] == '\\') {
        --top;
        pathcopy[top] = 0;
    }
    scan = top;

    // shorten the path 'till you find the first directory to be created 
    while (true) {
        if (mkdir(pathcopy, 0x1fd) == 0) break;
        if (errno == EEXIST) break;

        // we have more than one component non existing, skip the last one
        for (--scan; scan > 0 && !(pathcopy[scan] == '/' || pathcopy[scan] == '\\'); --scan);
        if (scan == 0) {
            return(-1);
        }
        pathcopy[scan] = 0;
    } 

    // now create the other directories
    while (scan < top) {
        pathcopy[scan] = '/';
        scan = (int)strlen(pathcopy);
        if (mkdir(pathcopy, 0x1fd) != 0) {
            return(-1);
        }
    }
    return(0);
}

#endif

// STRING FORMATTING 
std::string formatInt(const int64_t val, const int32_t field_len, const int32_t flags)
{
    char format[20];
    char printout[70];
    std::string result;
    int32_t fl = std::min(std::max(field_len, 1), 64);

    synthFormat(format, flags & ~f_uppercase, PRId64, fl);
    sprintf(printout, format, val);
    if (strlen(printout) > fl) {
        std::string result;
        addChars(&result, '#', fl);
        return(result);
    }
    return(std::string(printout));
}

std::string formatUnsigned(const uint64_t val, const int32_t field_len, const int32_t flags)
{
    char format[20];
    char printout[70];
    std::string result;
    int32_t fl = std::min(std::max(field_len, 1), 64);
    bool plus = (flags & f_dont_omit_plus) != 0;

    if (plus) {
        synthFormat(format, flags & ~(f_uppercase | f_dont_omit_plus), PRIu64, std::max(fl - 1, 1));
        printout[0] = '+';
        sprintf(printout + 1, format, val);
    } else {
        synthFormat(format, flags & ~f_uppercase, PRIu64, fl);
        sprintf(printout, format, val);
    }
    if (strlen(printout) > fl) {
        std::string result;
        addChars(&result, '#', fl);
        return(result);
    }
    return(std::string(printout));
}

std::string formatUnsignedHex(const uint64_t val, const int32_t field_len, const int32_t flags)
{
    char format[20];
    char printout[70];
    std::string result;
    int32_t fl = std::min(std::max(field_len, 1), 64);
    bool plus = (flags & f_dont_omit_plus) != 0;

    if (plus) {
        synthFormat(format, flags, PRIx64, std::max(fl - 1, 1));
        printout[0] = '+';
        sprintf(printout + 1, format, val);
    } else {
        synthFormat(format, flags, PRIx64, fl);
        sprintf(printout, format, val);
    }
    if (strlen(printout) > fl) {
        std::string result;
        addChars(&result, '#', fl);
        return(result);
    }
    return(std::string(printout));
}

std::string formatFloat(const double val, const int32_t field_len, const int32_t fract_digits, const int32_t flags)
{
    char format[20];
    char printout[600];
    std::string result;
    int32_t fl = std::min(std::max(field_len, 1), 64);

    synthFormat(format, flags, "f", fl, std::min(std::max(fract_digits, 0), 16));
    sprintf(printout, format, val);
    if (strlen(printout) > fl) {
        std::string result;
        addChars(&result, '#', fl);
        return(result);
    }
    return(std::string(printout));
}

std::string formatFloatSci(const double val, const int32_t field_len, const int32_t fract_digits, const int32_t flags)
{    
    return(formatFloatExp(val, 1, field_len, fract_digits, flags));
}

std::string formatFloatEng(const double val, const int32_t field_len, const int32_t fract_digits, const int32_t flags)
{
    return(formatFloatExp(val, 3, field_len, fract_digits, flags));
}

std::string formatFloatExp(const double val, int32_t exp_mult, const int32_t field_len, const int32_t fract_digits, const int32_t flags)
{
    int32_t exp = 0;
    double  vv = val;
    int32_t fl = std::min(std::max(field_len, 1), 64);
    int32_t fd = std::min(std::max(fract_digits, 0), 16);

    // special cases (nan and infinite)
    if (val != val || val * 0.1 == val) {
        return(formatFloat(val, field_len, fract_digits, flags));
    }

    // split base and exp
    if (val != 0) {
        double ee = log10(sing::abs(val));
        exp = (int)ee / exp_mult * exp_mult;
        if (ee < 0) {
            exp -= exp_mult;
        }
        vv = val * sing::exp10(-(double)exp);

        // chek if we are out of bounds because of rounding errors
        double max = 1;
        for (int ii = 0; ii < exp_mult; ++ii) max *= 10.0;
        double vvabs = sing::abs(vv);
        if (vvabs >= max) {
            vv /= max;
            exp += exp_mult;
        } else if (vvabs < 1.0) {
            vv *= max;
            exp -= exp_mult;
        }
    }

    char format[20];
    char printout[70];

    // printout as compact as possible !
    // note: in case of zero padding we must set the exact field len
    int len = 1;
    bool plus = (flags & f_dont_omit_plus) != 0;
    bool zeros = (flags & f_zero_prefix) != 0;
    if (zeros) {
        len = exp_mult;
        if (plus || vv < 0) ++len;  // sign
        if (fd > 0) len += fd + 1;  // fract part plus point.
    }
    synthFormat(format, flags & ~f_align_left, "f", len, fd);   // dont align left, else 0 padding is lost
    sprintf(printout, format, vv);
    strcat(printout, (flags & f_uppercase) != 0 ? "E" : "e");
    if (zeros) {
        sprintf(printout + strlen(printout), "%+04" PRId32, exp);
    } else {
        sprintf(printout + strlen(printout), "%+-4" PRId32, exp);     // 3 digits long to keep the vertical alignment
    }

    if (strlen(printout) > fl) {
        std::string result;
        addChars(&result, '#', fl);
        return(result);
    }

    // align
    return(formatString(printout, fl, flags));
}

std::string formatString(const char *val, const int32_t field_len, const int32_t flags)
{
    std::string result;
    bool align_left = (flags & f_align_left) != 0;

    int32_t padding = field_len - (int32_t)strlen(val);
    if (padding == 0 || field_len <= 0) return(std::string(val));
    if (padding < 0) {
        result = val;
        result.resize(field_len);
    } else {
        if (!align_left) {
            addBlanks(&result, padding);
        }
        result += val;
        if (align_left) {
            addBlanks(&result, padding);            
        }
    }
    return(result);
}

void addBlanks(std::string *dst, int32_t count)
{
    dst->reserve(dst->length() + count);
    while (count-- > 0) {
        *dst += ' ';
    }
}

void addChars(std::string *dst, char cc, int32_t count)
{
    dst->reserve(dst->length() + count);
    while (count-- > 0) {
        *dst += cc;
    }
}

void synthFormat(char *dst, int32_t flags, const char *base_conversion, int size, int fract)
{
    *dst++ = '%';
    if ((flags & f_dont_omit_plus) != 0) *dst++ = '+';
    if ((flags & f_align_left) != 0) *dst++ = '-';
    if ((flags & f_zero_prefix) != 0) *dst++ = '0';
    sprintf(dst, "%d", size);
    if (fract >= 0) {
        sprintf(dst + strlen(dst), ".%d", fract);
    }
    strcat(dst, base_conversion);
    if ((flags & f_uppercase) != 0) {
        dst[strlen(dst) - 1] -= 32;
    }
}

// string parsing
bool parseInt(int64_t *value, const char *from, const int32_t at, int32_t *last_pos)
{
    int pp;
    const char *src = from + at;
    if (*src == '0' && src[1] == 'x' || src[1] == 'X') {
        if (parseUnsignedHex((uint64_t*)value, from, at + 2, last_pos) && value >= 0) {
            return(true);
        }
        return(false);
    }
    if (sscanf(from + at, "%" SCNd64 "%n", value, &pp) != 1) {
        return(false);
    }
    *last_pos = (int32_t)pp + at;
    return(true);
}

bool parseUnsignedInt(uint64_t *value, const char *from, const int32_t at, int32_t *last_pos)
{
    int pp;
    const char *src = from + at;
    if (*src == '0' && src[1] == 'x' || src[1] == 'X') {
        if (parseUnsignedHex(value, from, at + 2, last_pos)) {
            return(true);
        }
        return(false);
    }
    if (sscanf(from + at, "%" SCNu64 "%n", value, &pp) != 1) {
        return(false);
    }
    *last_pos = (int32_t)pp + at;
    return(true);
}

bool parseUnsignedHex(uint64_t *value, const char *from, const int32_t at, int32_t *last_pos)
{
    int pp;
    if (sscanf(from + at, "%" SCNx64 "%n", value, &pp) != 1) {
        return(false);
    }
    *last_pos = (int32_t)pp + at;
    return(true);
}

bool parseFloat(double *value, const char *from, const int32_t at, int32_t *last_pos)
{
    int pp;
    if (sscanf(from + at, "%lf%n", value, &pp) != 1) {
        return(false);
    }
    *last_pos = (int32_t)pp + at;
    return(true);
}

// console
void print(const char *value)
{
    printf("%s", value);
}

void printError(const char *value)
{
    fprintf(stderr, "%s", value);
}

void scrClear()
{
#ifdef _WIN32
    ::system("cls");
#else    
    int dummy = ::system("clear");
#endif
}

#ifndef _WIN32
char _getch()
{
    struct termios oldt, newt;
    char ch;

    tcgetattr(0, &oldt);                 // grab old terminal i/o settings
    newt = oldt;                          // make new settings same as old settings
    newt.c_lflag &= ~(ICANON | ECHO);    // disable buffered i/o and echo
    tcsetattr(0, TCSANOW, &newt); 
    ch = getchar();
    tcsetattr(0, TCSANOW, &oldt); 
    return ch;    
}
#endif

std::string kbdGet()
{
    char buf[2];
    buf[0] = _getch();
    while (buf[0] == 0xe0) {
        _getch();
        buf[0] = _getch();
    }
    buf[1] = 0;
    return(std::string(buf));
}

std::string kbdInput(const int32_t max_digits)
{
    std::string result;
    int cc;

    do {
        cc = _getch();
        if (cc == '\b') {
            if (result.length() > 0) {
                result.pop_back();
                printf("\b \b");
            }
        } else if (cc == 0x1b) {
            for (int ii = 0; ii < result.length(); ++ii) printf("\b \b");
            result = "";
        } else if (cc == 0xe0) {
            _getch();
        } else if (cc >= ' ') {
            if (result.length() < max_digits) {
                result += cc;
                putchar(cc);
            }
        }
    } while (cc != '\r' && cc != '\n');
    return(result);
}

} // namespace
