#include <direct.h>
#include <string.h>
#include <Windows.h>
#include "target.h"

namespace SingNames {

const char *get_cwd(void)
{
    static char cwd_buffer[1024];
    static bool cwd_set = false;

    if (!cwd_set) {
        strcpy(cwd_buffer, "\\");
        _getcwd(cwd_buffer, 1023);
        cwd_set = true;
    }
    return(cwd_buffer);
}

int get_drive(void)
{
    static int drive = 0;
    static bool drv_set = false;
    if (!drv_set) {
        drv_set = true;
        drive = _getdrive();
    }
    return(drive);
}

bool is_same_file(const char *p0, const char *p1)
{
#ifdef _WIN32

    //Get file handles
    HANDLE handle1 = ::CreateFileA(p0, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE handle2 = ::CreateFileA(p1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    bool bResult = false;

    //if we could open both paths...
    if (handle1 != INVALID_HANDLE_VALUE && handle2 != INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION fileInfo1;
        BY_HANDLE_FILE_INFORMATION fileInfo2;
        if (::GetFileInformationByHandle(handle1, &fileInfo1) && ::GetFileInformationByHandle(handle2, &fileInfo2))
        {
            //the paths are the same if they refer to the same file (fileindex) on the same volume (volume serial number)
            bResult = fileInfo1.dwVolumeSerialNumber == fileInfo2.dwVolumeSerialNumber &&
                fileInfo1.nFileIndexHigh == fileInfo2.nFileIndexHigh &&
                fileInfo1.nFileIndexLow == fileInfo2.nFileIndexLow;
        }
    }

    //free the handles
    if (handle1 != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(handle1);
    }

    if (handle2 != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(handle2);
    }

    //return the result
    return bResult;
#else
    bool IsSameFile(const char* sA, const char* sB)
    {
        stat A = stat(sA);
        stat B = stat(sB);

        return A.st_dev == B.st_dev && A.st_ino == B.st_ino;
    }
#endif
}

bool is_same_filename(const char *name1, const char *name2)
{
    return(stricmp(name1, name2) == 0);
}

}   // namespace
