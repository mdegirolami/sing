#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include "NamesList.h"
#include "vector.h"

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

class ErrorList {
public:
    void AddError(const char *message, int nRow, int nCol);
    const char *GetError(int index, int *nRow, int *nCol) const;
    int NumErrors(void) const { return((int)rows_.size()); }
    void Reset(void);
    void Append(const ErrorList *source);
    void Sort(void);
    int CompareForSort(int a, int b);

private:
    NamesList   errors_strings_;
    vector<int> rows_;
    vector<int> cols_;
};

void quick_sort_indices(int *vv, int count, int(*comp)(int, int, void *), void *context);

} // namespace

#endif
