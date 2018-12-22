#ifndef __NAMESLIST_H_
#define __NAMESLIST_H_

#include "vector.h"

namespace SingNames {

class NamesList {
    vector<char>    _names;
    vector<uint32_t>   _indices;

    void TerminateAndCreateIndices(void);
public:
    void AddName(const char *name);
    void AddCrLfTerminatedName(const char *name);
    void Sort(void);
    void DeleteDuplicated(void);
    const char *GetName(uint32_t index);
    int  CompareStrings(uint32_t i1, uint32_t i2);
    int  GetNamesCount(void) { return(_indices.size()); }
    void Reset(void) { _names.clear();_indices.clear(); }
    void Erase(int first, int past_last);
    int  SortedInsert(const char *name);                                // returns the inserted position

    // works after DeleteDuplicated and before any other AddName
    int  BinarySearchNearestLower(const char *name);                    // may return -1 if not found, a perfect match or the nearest item.
    int  BinarySearchNearestHigher(const char *name);                   // may return -1 if not found, a perfect match or the nearest item.
    int  BinarySearch(const char *name);                                // returns -1 if not found
    int  BinarySearchRangeWithPrefix(int &index, const char *prefix);   // returns the number of matches
    int  LinearSearch(const char *name);
    bool IsEqualTo(NamesList *other);
};

}
#endif