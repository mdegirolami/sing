#ifndef __FILENAME_H_
#define __FILENAME_H_

#include "string"

namespace SingNames {

class FileName {
    static int SearchLastSlash(const char *name);

public:
    // name and exp can be full names, the appropriate part is extracted !
    static void BuildFullName(const string *path, const string *name,
        const string *exp, string *fullname);

    // output parms dont include \ and . separators
    // path, name, exp may be NULL (if the return value is not needed)
    static void SplitFullName(string *path, string *name,
        string *exp, const string *fullname);

    // the parameter 'extension' can be with or without dot.
    static bool ExtensionIs(const string *filename, const string *extension);
    static void ExtensionSet(string *filename, const string *extension);
    static void ExtensionSet(string *filename, const char *extension);

    static void FixBackSlashes(string *filename) { filename->replace('\\', '/'); }
};

}   // namespace
#endif