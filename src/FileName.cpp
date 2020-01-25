#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <stdio.h>
#endif
#include <string.h>
#include "FileName.h"
#include "target.h"

namespace SingNames {

void FileName::BuildFullName(const string *path, const string *name,
    const string *exp, string *fullname)
{
    URGH_SIZE_T     dot, bkslash;
    char            lastchar;

    // append the \ terminator to the name if required
    *fullname = *path;
    lastchar = fullname->c_str()[fullname->size() - 1];
    if (lastchar != '\\' && lastchar != '/') {
        (*fullname) += '/';
    }

    // find where the name starts/ends inside the name string
    dot = name->rfind('.');
    if (dot == string::npos) dot = name->size();

    bkslash = SearchLastSlash(name->c_str());
    (*fullname) += name->substr(bkslash, (DWORD)(dot - bkslash));
    ExtensionSet(fullname, exp);
}

// actually returns the first valid char in the last pathname component (the file name)
int FileName::SearchLastSlash(const char *name)
{
    const char  *bkslash;

    bkslash = name + strlen(name);
    while (bkslash > name) {
        --bkslash;
        if (*bkslash == '\\' || *bkslash == '/') {
            ++bkslash;
            break;
        }
    }
    return((int)(bkslash - name));
}

void FileName::SplitFullName(string *path, string *name,
    string *exp, const string *fullname)
{
    URGH_SIZE_T  dot, slash, top;

    // extract exp
    top = fullname->size();
    dot = fullname->rfind('.');
    if (dot == string::npos) {
        if (exp != NULL) {
            *exp = "";
        }
    } else {
        if (exp != NULL) {
            *exp = fullname->c_str() + dot + 1;
        }
        top = dot;
    }
    if (path == NULL && name == NULL) return;

    // split name and path
    slash = SearchLastSlash(fullname->c_str());
    if (slash == 0) {
        if (path != NULL) {
            *path = "";
        }
        if (name != NULL) {
            *name = fullname->substr(0, top);
        }
    } else {
        if (path != NULL) {
            *path = fullname->substr(0, slash - 1);
        }
        if (name != NULL) {
            *name = fullname->substr(slash, top);
        }
    }
}

bool FileName::ExtensionIs(const string *filename, const string *extension)
{
    string  ext1, ext2;

    SplitFullName(NULL, NULL, &ext1, filename);
    SplitFullName(NULL, NULL, &ext2, extension);
    return(is_same_filename(ext1.c_str(), ext2.c_str()));
}

void FileName::ExtensionSet(string *filename, const string *extension)
{
    URGH_SIZE_T  dot;

    // erase current extension
    dot = filename->rfind('.');
    if (dot != string::npos) {
        filename->erase(dot);
    }

    // if no extension we are done
    if (extension == NULL || *extension == "") return;

    // add the new extension, also add the dot if not from 'exp'  
    dot = extension->rfind('.');
    if (dot == string::npos) {
        (*filename) += '.';
        (*filename) += *extension;
    } else {
        (*filename) += extension->c_str() + dot;
    }
}

void FileName::ExtensionSet(string *filename, const char *extension)
{
    URGH_SIZE_T  dot;
    const char   *cdot;

    // erase current extension
    dot = filename->rfind('.');
    if (dot != string::npos) {
        filename->erase(dot);
    }

    // if no extension we are done
    if (extension == NULL || *extension == 0) return;

    // add the new extension, also add the dot if not from 'exp'  
    cdot = strrchr(extension, '.');
    if (cdot == NULL) {
        (*filename) += '.';
        (*filename) += extension;
    } else {
        (*filename) += cdot;
    }
}

} // namespace

/* forbidden

< (less than)
> (greater than)
: (colon - sometimes works, but is actually NTFS Alternate Data Streams)
" (double quote)
/ (forward slash)
\ (backslash)
| (vertical bar or pipe)
? (question mark)
* (asterisk)

~ e' speciale in linux !!!

*/