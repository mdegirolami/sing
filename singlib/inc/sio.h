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
    virtual Error gets(int64_t maxbytes, std::string *value) = 0;
    virtual Error read(int64_t count, std::vector<uint8_t> *dst, bool append = true) = 0;

    virtual Error put(uint8_t value) = 0;
    virtual Error puts(const char *value) = 0;
    virtual Error write(int64_t count, const std::vector<uint8_t> &src, int64_t from = 0) = 0;

    virtual Error seek(int64_t pos, SeekMode mode = SeekMode::seek_set) = 0;
    virtual Error tell(int64_t *pos) = 0;

    virtual bool eof() const = 0;
    virtual Error close() = 0;
};

class FileInfo final {
public:
    FileInfo();
    uint64_t last_modification_time_;
    int64_t length_;
    bool is_dir_;
    bool is_read_only_;
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
    Error flush();
    Error getInfo(FileInfo *nfo);
    virtual Error get(uint8_t *value) override;
    virtual Error gets(int64_t maxbytes, std::string *value) override;
    virtual Error read(int64_t count, std::vector<uint8_t> *dst, bool append = true) override;

    virtual Error put(uint8_t value) override;
    virtual Error puts(const char *value) override;
    virtual Error write(int64_t count, const std::vector<uint8_t> &src, int64_t from = 0) override;

    virtual Error seek(int64_t pos, SeekMode mode = SeekMode::seek_set) override;
    virtual Error tell(int64_t *pos) override;

    virtual bool eof() const override;
    virtual Error close() override;

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
Error fileWriteProtect(const char *filename, bool protect);
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
std::string pathJoinAll(int32_t drive, bool absolute, const std::vector<std::string> &parts);
std::string pathFix(const char *path);
void pathSetExtension(std::string *path, const char *ext);
bool pathToAbsolute(std::string *result, const char *relpath, const char *basepath = "");
bool pathToRelative(std::string *result, const char *abspath, const char *basepath = "");
bool pathIsAbsolute(const char *path);
int32_t pathGetDriveIdx(const char *path);

// dirs
Error dirRead(const char *directory, DirFilter filter, std::vector<std::string> *names, std::vector<FileInfo> *info, bool recursive = false);
Error dirReadNames(const char *directory, DirFilter filter, std::vector<std::string> *names, bool recursive = false);
Error dirRemove(const char *directory, bool if_not_empty = false);
Error dirCreate(const char *directory);

// string formatting 
std::string formatInt(int64_t val, int32_t field_len, int32_t flags = 0);
std::string formatUnsigned(uint64_t val, int32_t field_len, int32_t flags = 0);
std::string formatUnsignedHex(uint64_t val, int32_t field_len, int32_t flags = 0);
std::string formatFloat(double val, int32_t field_len, int32_t fract_digits = 6, int32_t flags = 0);
std::string formatFloatSci(double val, int32_t field_len, int32_t fract_digits = 6, int32_t flags = 0);
std::string formatFloatEng(double val, int32_t field_len, int32_t fract_digits = 6, int32_t flags = 0);
std::string formatString(const char *val, int32_t field_len, int32_t flags = 0);

// string parsing
bool parseInt(int64_t *value, const char *from, int32_t at, int32_t *last_pos);
bool parseUnsignedInt(uint64_t *value, const char *from, int32_t at, int32_t *last_pos);
bool parseUnsignedHex(uint64_t *value, const char *from, int32_t at, int32_t *last_pos);
bool parseFloat(double *value, const char *from, int32_t at, int32_t *last_pos);

// console
void print(const char *value);
void printError(const char *value);
void scrClear();
std::string kbdGet();
std::string kbdInput(int32_t max_digits = 0);

}   // namespace
