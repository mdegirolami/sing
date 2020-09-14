#pragma once

#include <sing.h>

namespace sing {

// Stream interface is here to access files / memory or other sources in a uniform way. 
typedef int64_t Error;

enum class SeekMode {seek_set, seek_cur, seek_end};

class Stream {
public:
    virtual ~Stream() {}
    virtual void *get__id() const = 0;
    virtual Error get(uint8_t *value) = 0;
    virtual Error gets(const int64_t maxbytes, std::string *value) = 0;
    virtual Error read(const int64_t count, std::vector<uint8_t> *dst, const bool append = true) = 0;
    virtual Error put(const uint8_t value) = 0;
    virtual Error puts(const char *value) = 0;
    virtual Error write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from = 0) = 0;
    virtual Error seek(const int64_t pos, const SeekMode &mode = SeekMode::seek_set) = 0;
    virtual Error tell(int64_t *pos) = 0;
    virtual bool eof() const = 0;
};


class FileInfo final {
public:
    FileInfo();
    uint64_t last_modification_time_;
    int64_t length_;
    bool is_dir_;
};

// open modes are modes: "r", "w", "r+", "w+"
class File final : public Stream {
    void    *fd_;
    int     dir_;
public:
    File();
    virtual ~File();
    virtual void *get__id() const override { return(&id__); };
    Error open(const char *name, const char *mode);
    Error close();
    Error flush();
    Error getInfo(FileInfo *nfo);
    virtual Error get(uint8_t *value) override;
    virtual Error gets(const int64_t maxbytes, std::string *value) override;
    virtual Error read(const int64_t count, std::vector<uint8_t> *dst, const bool append = true) override;
    virtual Error put(const uint8_t value) override;
    virtual Error puts(const char *value) override;
    virtual Error write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from = 0) override;
    virtual Error seek(const int64_t pos, const SeekMode &mode = SeekMode::seek_set) override;
    virtual Error tell(int64_t *pos) override;
    virtual bool eof() const override;

    static char id__;
};

// filtering option for dirRead
enum class DirFilter {regular, directory, all};

// string formatting flags/options 
static const int32_t f_dont_omit_plus = 1;
static const int32_t f_zero_prefix = 2;
static const int32_t f_uppercase = 4;
static const int32_t f_align_left = 8;

// files free functions    
Error fileRemove(const char *filename);
Error fileRename(const char *old_name, const char *new_name);
Error fileGetInfo(const char *filename, FileInfo *nfo);
Error fileRead(const char *filename, std::vector<uint8_t> *dst);
Error fileReadText(const char *filename, std::string *dst);
Error fileWrite(const char *filename, const std::vector<uint8_t> &src);
Error fileWriteText(const char *filename, const char *src);

// paths    
Error setCwd(const char *directory);
std::string getCwd();
void pathSplit(const char *fullname, std::string *drive, std::string *path, std::string *base, std::string *extension);
std::string pathJoin(const char *drive, const char *path, const char *base, const char *extension);
void pathSplitAll(const char *fullname, int32_t *drive, bool *absolute, std::vector<std::string> *parts);
std::string pathJoinAll(const int32_t drive, const bool absolute, const std::vector<std::string> &parts);
std::string pathFix(const char *path);
void pathSetExtension(std::string *path, const char *ext);
bool pathToAbsolute(std::string *result, const char *relpath, const char *basepath = "");
bool pathToRelative(std::string *result, const char *abspath, const char *basepath = "");
bool pathIsAbsolute(const char *path);
int32_t pathGetDriveIdx(const char *path);

// dirs
Error dirRead(const char *directory, const char *namespec, const DirFilter &filter, std::vector<std::string> *names, std::vector<FileInfo> *info,
    const bool recursive = false);
Error dirReadNames(const char *directory, const char *namespec, const DirFilter &filter, std::vector<std::string> *names, const bool recursive = false);
Error dirRemove(const char *directory, const bool if_not_empty = false);
Error dirCreate(const char *directory);

// string formatting 
std::string formatInt(const int64_t val, const int32_t field_len, const int32_t flags = 0);
std::string formatUnsigned(const uint64_t val, const int32_t field_len, const int32_t flags = 0);
std::string formatUnsignedHex(const uint64_t val, const int32_t field_len, const int32_t flags = 0);
std::string formatFloat(const double val, const int32_t field_len, const int32_t fract_digits = 6, const int32_t flags = 0);
std::string formatFloatSci(const double val, const int32_t field_len, const int32_t fract_digits = 6, const int32_t flags = 0);
std::string formatFloatEng(const double val, const int32_t field_len, const int32_t fract_digits = 6, const int32_t flags = 0);
std::string formatString(const char *val, const int32_t field_len, const int32_t flags = 0);

// string parsing
bool parseInt(int64_t *value, const char *from, const int32_t at, int32_t *last_pos);
bool parseUnsignedInt(uint64_t *value, const char *from, const int32_t at, int32_t *last_pos);
bool parseUnsignedHex(uint64_t *value, const char *from, const int32_t at, int32_t *last_pos);
bool parseFloat(double *value, const char *from, const int32_t at, int32_t *last_pos);

// console
void print(const char *value);
void printError(const char *value);
void scrClear();
std::string kbdGet();
std::string kbdInput(const int32_t max_digits = 0);

}   // namespace
