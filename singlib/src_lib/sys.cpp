#include "sys.h"
#include <time.h>
#include <assert.h>

#ifdef _WIN32    
#include <windows.h>
#endif

namespace sing {

////////////////////////
//
// WIN32 VERSIONS
//
///////////////////////
#ifdef _WIN32    

// implemented in str.cpp
std::string utf16_to_8(const wchar_t *src);
void utf8_to_16(const char *src, std::vector<wchar_t> *dst);

// PROCESSES
void system(const char *command)
{
    std::vector<wchar_t> wcommand;
    utf8_to_16(command, &wcommand);
    ::_wsystem(wcommand.data());
}

Phandle execute(const char *command)
{
    std::vector<wchar_t> wcommand;
    utf8_to_16(command, &wcommand);

    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFOW siStartInfo;
    BOOL bSuccess = FALSE; 
 
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO); 

    bSuccess = CreateProcessW(&wcommand[0], 
      NULL,  // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      FALSE,         // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
    if ( !bSuccess ) return(0);
    return((Phandle)piProcInfo.hProcess);
}

class Pipe final : public Stream {    
#ifdef _WIN32
    HANDLE hh_;
#else
#endif
public:
    Pipe() { hh_ = nullptr; }
    void SetHandle(HANDLE hh) { hh_ = hh; }
    virtual ~Pipe() {if (hh_ != nullptr) CloseHandle(hh_); }
    virtual void *get__id() const override { return(nullptr); }    // unknown type for sing !!
    virtual Error get(uint8_t *value) override;
    virtual Error gets(const int64_t maxbytes, std::string *value) override;
    virtual Error read(const int64_t count, std::vector<uint8_t> *dst, const bool append = true) override;
    virtual Error put(const uint8_t value) override;
    virtual Error puts(const char *value) override;
    virtual Error write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from = 0) override;
    virtual Error seek(const int64_t pos, const SeekMode &mode = SeekMode::seek_set) override { return(-1); }
    virtual Error tell(int64_t *pos) override { return(-1); }
    virtual bool eof() const override;
};

Error Pipe::get(uint8_t *value)
{
    DWORD dwRead;

    if (!ReadFile(hh_, value, 1, &dwRead, NULL) || dwRead == 0 ) return(-1);
    return(0);
}

Error Pipe::gets(const int64_t maxbytes, std::string *value)
{
    uint8_t cc = 0;
    *value = "";
    do {
        if (get(&cc) != 0) return(-1);
        if (cc != 0) *value += cc;
    } while (cc != '\n' && cc != 0 && (int64_t)value->length() < maxbytes);
    size_t len = value->length();
    if (len > 1 && value->at(len - 1) == '\n' && value->at(len - 2) == '\r') {
        value->resize(len - 2);
        *value += '\n';
    }
    return(0);
}

Error Pipe::read(const int64_t count, std::vector<uint8_t> *dst, const bool append)
{
    DWORD dwRead;
    size_t pos;

    if (count < 0) return(-1);
    if (append) {
        pos = dst->size();
        dst->resize(pos + (size_t)count);
    } else {
        pos = 0;
        dst->clear();
        dst->resize(count);
    }
    if (!ReadFile(hh_, &(*dst)[pos], count, &dwRead, NULL) || (int64_t)dwRead != count) return(-1);
    return(0);
}

Error Pipe::put(const uint8_t value)
{
    DWORD dwWritten;
    if (!WriteFile(hh_, &value, 1, &dwWritten, NULL)) return(-1);
    return(0);
}

Error Pipe::puts(const char *value)
{
    DWORD dwWritten;
    if (!WriteFile(hh_, value, strlen(value), &dwWritten, NULL)) return(-1);
    return(0);
}

Error Pipe::write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from)
{
    DWORD dwWritten;
    if (from < 0) return(-1);
    int64_t towrite = std::min(count, (int64_t)src.size() - from);
    if (towrite > 0) {
        if (!WriteFile(hh_, src.data(), src.size(), &dwWritten, NULL)) return(-1);
    }
    return(0);
}

bool Pipe::eof() const
{
    return(false);
}

Phandle automate(const char *command, std::shared_ptr<Stream> *sstdin, std::shared_ptr<Stream> *sstdout, std::shared_ptr<Stream> *sstderr)
{
    std::vector<wchar_t> wcommand;
    utf8_to_16(command, &wcommand);
    std::shared_ptr<Pipe> pin = std::make_shared<Pipe>();
    std::shared_ptr<Pipe> pout = std::make_shared<Pipe>();
    std::shared_ptr<Pipe> perr = std::make_shared<Pipe>();

    // craete the pipes
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    HANDLE childout_rd = nullptr;
    HANDLE childout_wr = nullptr;
    HANDLE childin_rd = nullptr;
    HANDLE childin_wr = nullptr;
    HANDLE childerr_rd = nullptr;
    HANDLE childerr_wr = nullptr;  
    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFOW siStartInfo;
    BOOL bSuccess = FALSE; 
 
    if (!CreatePipe(&childout_rd, &childout_wr, &saAttr, 0)) {
        goto cleanup;
    } 
    if (!SetHandleInformation(childout_rd, HANDLE_FLAG_INHERIT, 0)) { // forbit inheritance
        goto cleanup;
    }

    if (!CreatePipe(&childin_rd, &childin_wr, &saAttr, 0)) {
        goto cleanup;
    } 
    if (!SetHandleInformation(childin_wr, HANDLE_FLAG_INHERIT, 0)) { // forbit inheritance
        goto cleanup;
    }

    if (!CreatePipe(&childerr_rd, &childerr_wr, &saAttr, 0)) {
        goto cleanup;
    } 
    if (!SetHandleInformation(childerr_rd, HANDLE_FLAG_INHERIT, 0)) { // forbit inheritance
        goto cleanup;
    }

    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.hStdError = childerr_wr;
    siStartInfo.hStdOutput = childout_wr;
    siStartInfo.hStdInput = childin_rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
    bSuccess = CreateProcessW(NULL, 
      &wcommand[0],     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
    if (! bSuccess ) goto cleanup;

    // Close handles to the child process and its primary thread.
    CloseHandle(piProcInfo.hThread);
      
    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not explicitly closed, there is no way to recognize that the child process has ended.
    CloseHandle(childerr_wr);
    CloseHandle(childout_wr);
    CloseHandle(childin_rd);

    (*pin).SetHandle(childin_wr);
    (*pout).SetHandle(childout_rd);
    (*perr).SetHandle(childerr_rd);

    *sstdin = pin;
    *sstdout = pout;
    *sstderr = perr;

    return((Phandle)piProcInfo.hProcess);

cleanup:
    if (childout_rd != nullptr) CloseHandle(childout_rd);
    if (childout_wr != nullptr) CloseHandle(childout_wr);
    if (childerr_rd != nullptr) CloseHandle(childerr_rd);
    if (childerr_wr != nullptr) CloseHandle(childerr_wr);
    if (childin_rd  != nullptr) CloseHandle(childin_rd);
    if (childin_wr  != nullptr) CloseHandle(childin_wr);
    return(0);
}

int32_t waitCommandExit(const Phandle handle)
{
    DWORD ecode = 0;
    WaitForSingleObject((HANDLE)handle, INFINITE);
    GetExitCodeProcess((HANDLE)handle, &ecode);
    return((int32_t)ecode);
}

void exit(const int32_t retcode)
{
    ::exit(retcode);
}


// ENVIRONMENT
std::string getenv(const char *name)
{
    std::vector<wchar_t> wkey;
    std::vector<wchar_t> value;

    utf8_to_16(name, &wkey);
    DWORD length = GetEnvironmentVariableW(wkey.data(), NULL, 0);
    if (length == 0) return("");
    value.reserve(length + 1);
    if (GetEnvironmentVariableW(wkey.data(), value.data(), length + 1) == 0) {
        return("");
    }
    return(utf16_to_8(value.data()));
}

void setenv(const char *name, const char *value, const bool override)
{
    std::vector<wchar_t> wkey;
    std::vector<wchar_t> wval;
    utf8_to_16(name, &wkey);
    utf8_to_16(value, &wval);
    if (override || GetEnvironmentVariableW(wkey.data(), NULL, 0) == 0) {
        SetEnvironmentVariableW(wkey.data(), wval.data());
    }
}

// time management
void wait(const int32_t microseconds)
{
    ::Sleep(microseconds / 1000);
}

int64_t time()
{
    return((int64_t)::time(nullptr));
}

int64_t clock()
{
    LARGE_INTEGER count;

    QueryPerformanceCounter(&count);
    return(count.QuadPart);
}

int64_t clocksDiff(const int64_t before, const int64_t after)
{
    static LARGE_INTEGER frequency;
    static bool frequency_known = false;

    if (!frequency_known) {
        QueryPerformanceFrequency(&frequency);
        frequency_known = true;
    }
    return((after - before) * 1000000 / frequency.QuadPart);
}

#else

////////////////////////
//
// LINUX/UNIX VERSIONS
//
///////////////////////

// PROCESSES
void exec(const char *command);

void system(const char *command)
{
    ::system(command);
}

Phandle execute(const char *command)
{
    int nChild;

    nChild = fork();
    if (0 == nChild) {
        // child continues here
        exec(command);
    } else if (nChild > 0) {
        // parent continues here
        return(nChild);
    }
    return(0);
}

void exec(const char *command)
{
    std::vector<char> commandv;
    std::vector<char *const> argv;

    commandv.reserve(strlen(command) + 1);
    argv.reserve(16);
    bool quoted = false;
    bool separator = true;
    while (*command != 0) {
        if (separator) {
            if (*command != ' ' && *command != '\t') {
                if (*command == '"') {
                    quoted = true;
                } else {
                    commandv.push_back(*command);
                    argv.push_back(&commandv.back());
                }
                separator == false;
            }
        } else if (quoted) {
            if (*command == '"') {
                quoted = false;
            } else {
                commandv.push_back(*command);
            }
        } else {
            if (*command == ' ' || *command == '\t') {
                commandv.push_back(0);
                separator = true;
            } else {
                commandv.push_back(*command);
            }
        }
        command++;
    }
    commandv.push_back(0);
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());

    // if we are here exec failed
    ::exit();
}

class Pipe final : public Stream {    
#ifdef _WIN32
    int hh_;
#else
#endif
public:
    Pipe() { hh_ = -1; }
    void SetHandle(int hh) { hh_ = hh; }
    virtual ~Pipe() {if (hh_ != -1) close(hh_); }
    virtual void *get__id() const override { return(nullptr); }    // unknown type for sing !!
    virtual Error get(uint8_t *value) override;
    virtual Error gets(const int64_t maxbytes, std::string *value) override;
    virtual Error read(const int64_t count, std::vector<uint8_t> *dst, const bool append = true) override;
    virtual Error put(const uint8_t value) override;
    virtual Error puts(const char *value) override;
    virtual Error write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from = 0) override;
    virtual Error seek(const int64_t pos, const SeekMode &mode = SeekMode::seek_set) override { return(-1); }
    virtual Error tell(int64_t *pos) override { return(-1); }
    virtual bool eof() const override;
};

Error Pipe::get(uint8_t *value)
{
    if (read(hh_, &value, 1) != 1) {
        return(-1);
    }
    return(0);
}

Error Pipe::gets(const int64_t maxbytes, std::string *value)
{
    uint8_t cc = 0;
    *value = "";
    do {
        if (get(&cc) != 0) return(-1);
        if (cc != 0) *value += cc;
    } while (cc != '\n' && cc != 0 && (int64_t)value->length() < maxbytes);
    size_t len = value->length();
    if (len > 1 && value->at(len - 1) == '\n' && value->at(len - 2) == '\r') {
        value->resize(len - 2);
        *value += '\n';
    }
    return(0);
}

Error Pipe::read(const int64_t count, std::vector<uint8_t> *dst, const bool append)
{
    size_t pos;

    if (count < 0) return(-1);
    if (append) {
        pos = dst->size();
        dst->resize(pos + (size_t)count);
    } else {
        pos = 0;
        dst->clear();
        dst->resize(count);
    }
    if (read(hh_, &(*dst)[pos], count) != count) {
        return(-1);
    }
    return(0);
}

Error Pipe::put(const uint8_t value)
{
    if (write(hh_, &value, 1) != 1) {
        return(-1);
    }
    return(0);
}

Error Pipe::puts(const char *value)
{
    size_t towrite = strlen(value);

    if (towrite < 1) return(0);
    if (write(hh_, &value, towrite) != towrite) {
        return(-1);
    }
    return(0);
}

Error Pipe::write(const int64_t count, const std::vector<uint8_t> &src, const int64_t from)
{
    DWORD dwWritten;
    if (from < 0) return(-1);
    int64_t towrite = std::min(count, (int64_t)src.size() - from);
    if (towrite > 0) {
        if (write(hh_, src.data(), src.size()) != src.size()) {
            return(-1);
        }
    }
    return(0);
}

bool Pipe::eof() const
{
    return(false);
}

Phandle automate(const char *command, sing::iptr<Stream> *sstdin, sing::iptr<Stream> *sstdout, sing::iptr<Stream> *sstderr)
{
    static const int pipe_read  = 0;
    static const int pipe_write = 1;

    int aStdinPipe[2] = {-1, -1};
    int aStdoutPipe[2] = {-1, -1};
    int aStderrPipe[2] = {-1, -1};
    int nChild;
    char nChar;
    int nResult;

    if (pipe(aStdinPipe) < 0) {
        goto cleanup;
    }
    if (pipe(aStdoutPipe) < 0) {
        goto cleanup;
    }
    if (pipe(aStderrPipe) < 0) {
        goto cleanup;
    }

    nChild = fork();
    if (0 == nChild) {
        // child continues here

        // redirect stdin
        if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1) {
            exit(errno);
        }

        // redirect stdout
        if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
            exit(errno);
        }

        // redirect stderr
        if (dup2(aStderrPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
            exit(errno);
        }

        // all these are for use by parent only
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        close(aStdoutPipe[PIPE_READ]);
        close(aStdoutPipe[PIPE_WRITE]); 
        close(aStderrPipe[PIPE_READ]);
        close(aStderrPipe[PIPE_WRITE]); 

        // run child process image
        exec(command);

    } else if (nChild > 0) {
        // parent continues here

        // close unused file descriptors, these are for child only
        close(aStdinPipe[PIPE_READ]);
        close(aStdoutPipe[PIPE_WRITE]); 
        close(aStderrPipe[PIPE_WRITE]); 

        (*pin).SetHandle(aStdinPipe[PIPE_WRITE]);
        (*pout).SetHandle(aStdoutPipe[PIPE_READ]);
        (*perr).SetHandle(aStderrPipe[PIPE_READ]);

        *sstdin = pin;
        *sstdout = pout;
        *sstderr = perr;

        return(nChild);
    }
cleanup:

    if (aStdinPipe[PIPE_READ] != -1) close(aStdinPipe[PIPE_READ]);
    if (aStdinPipe[PIPE_WRITE] != -1) close(aStdinPipe[PIPE_WRITE]);
    if (aStdoutPipe[PIPE_READ] != -1) close(aStdoutPipe[PIPE_READ]);
    if (aStdoutPipe[PIPE_WRITE] != -1) close(aStdoutPipe[PIPE_WRITE]);
    if (aStderrPipe[PIPE_READ] != -1)  close(aStderrPipe[PIPE_READ]);
    if (aStderrPipe[PIPE_WRITE] != -1) close(aStderrPipe[PIPE_WRITE]);
    return 0;
}

int32_t waitCommandExit(const Phandle handle)
{
    if (handle > 0) {
        int status;
        waitpid(handle, &status, 0);
        if (WIFEXITED(status)) { 
        reuturn(WEXITSTATUS(status));             
    }
    return(0);
}

void exit(const int32_t retcode)
{
    ::exit(retcode);
}

// ENVIRONMENT
std::string getenv(const char *name)
{
    char *value = ::getenv(name);
    if (value == nullptr) {
        return("");
    }
    return(value);
}

void setenv(const char *name, const char *value, const bool override)
{
    ::setenv(key, value, override ? 1 : 0);
}

// time management
void wait(const int32_t microseconds)
{
    timespec tts;
    
    tts.tv_sec = microseconds / 1000000;
    tts.tv_nsec = (microseconds - tts.tv_sec * 1000000) * 1000;
    nanosleep(&tts, nullptr);
}

int64_t time()
{
    return((int64_t)::time(nullptr));
}

int64_t clock()
{
    timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);
    return(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

int64_t clocksDiff(const int64_t before, const int64_t after)
{
    return(after - before);
}

#endif

BrokenTime::BrokenTime()
{
    second_ = 0;
    minute_ = 0;
    hour_ = 0;
    mday_ = 0;
    mon_ = 0;
    wday_ = 0;
    yday_ = 0;
    savings_ = false;
    year_ = 0;
}

void BrokenTime::fillLocal(const int64_t time)
{
    struct tm broken;
    time_t tt = (time_t)time;
    localtime_s(&broken, &tt);
    second_ = broken.tm_sec;
    minute_ = broken.tm_min;
    hour_ = broken.tm_hour;
    mday_ = broken.tm_mday;
    mon_ = broken.tm_mon;
    wday_ = broken.tm_wday;
    yday_ = broken.tm_yday;
    savings_ = broken.tm_isdst > 0;
    year_ = broken.tm_year + 1900;
}

void BrokenTime::fillUtc(const int64_t time)
{
    struct tm broken;
    time_t tt = (time_t)time;
    gmtime_s(&broken, &tt);
    second_ = broken.tm_sec;
    minute_ = broken.tm_min;
    hour_ = broken.tm_hour;
    mday_ = broken.tm_mday;
    mon_ = broken.tm_mon;
    wday_ = broken.tm_wday;
    yday_ = broken.tm_yday;
    savings_ = broken.tm_isdst > 0;
    year_ = broken.tm_year + 1900;
}

// random numbers
void RndGen::rndSeed(const int64_t seed)
{
    seed_ = (uint64_t)seed;
}

uint64_t RndGen::rndU64()
{
    seed_ = seed_ * 6364136223846793005 + 1442695040888963407;
    return(seed_);
}

RndGen::RndGen()
{
    seed_ = 0;
}

double RndGen::rnd()
{        
    double val = (double)(rndU64() &  0x3fffffffffffffffL) / (double)0x4000000000000000L;
    return(val == 1.0f ? 0.0f : val);   // wrap around
}

double RndGen::rndNorm()
{
    for (int attempt = 200; attempt > 0; --attempt) {
        double val = rnd() * 14 - 7;    // -7..7 values > 7 are about once in 10^22 cases, we ignore them.

        // based on the value, reduce the likelihood to be returned.
        // (discard it with a probability which increses with value)
        if (rnd() < exp(-0.5 * val * val)) {
            return(val);
        }
    }    
    return(0);    // this happens about once in 10^14 calls
}

// mix
void validate(const bool condition)
{
    assert(condition);
}

} // namespace
