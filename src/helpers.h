#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>

#define FAIL -1
#define MIN(a, b) ((a)>(b)?(b):(a))
#define MAX(a, b) ((a)>(b)?(a):(b))

namespace SingNames {


struct ParsingException {
public:
    int         number;
    int         row;
    int         column;
    const char *description;

    ParsingException(int n, int r, int c, const char *s) : number(n), row(r), column(c), description(s) {}
};

} // namespace

#endif
