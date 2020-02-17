#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string"
#include "NamesList.h"

namespace SingNames {

static NamesList *g_list;

static int g_NamesList_Stringcomp(const void *e1, const void *e2)
{
    return(g_list->CompareStrings(*(uint32_t*)e1, *(uint32_t*)e2));
}

void NamesList::AddName(const char *name)
{
    const char  *scan;

    _indices.push_back(_names.size());
    for (scan = name; *scan; ++scan) {
        _names.push_back(*scan);
    }
    _names.push_back(0);
}

void NamesList::AddCrLfTerminatedName(const char *name)
{
    const char  *scan;

    _indices.push_back(_names.size());
    for (scan = name; *scan && *scan != '\r' && *scan != '\n'; ++scan) {
        _names.push_back(*scan);
    }
    _names.push_back(0);
}

int NamesList::CompareStrings(uint32_t i1, uint32_t i2)
{
    return(stricmp(&_names[i1], &_names[i2]));
}

void NamesList::Erase(int first, int past_last)
{
    if (first < 0) first = 0;
    if (first >= (int)_indices.size() || past_last < 1 || first >= past_last) return;

    if (past_last >= (int)_indices.size()) {
        _names.erase(_indices[first], _names.size());
        _indices.erase(first, _indices.size());
    } else {
        int erase_from = _indices[first];
        int erase_to = _indices[past_last];
        int delta = erase_to - erase_from;
        int ii;

        _names.erase(erase_from, erase_to);
        for (ii = past_last; ii < (int)_indices.size(); ++ii) {
            _indices[ii] -= delta;
        }
        _indices.erase(first, past_last);
    }
}

void NamesList::Sort(void)
{
    int     count;

    count = _indices.size();
    if (count == 0) return;
    g_list = this;
    qsort(&_indices[0], count, sizeof(uint32_t), &g_NamesList_Stringcomp);
}

void NamesList::DeleteDuplicated(void)
{
    int     count, dst, ii;
    char    *oldstring;

    if (_indices.size() == 0) return;

    Sort();

    // compact vector avoiding duplicates
    count = _indices.size();
    oldstring = &_names[_indices[0]];
    dst = 1;
    for (ii = 1; ii < count; ++ii) {
        if (stricmp(oldstring, &_names[_indices[ii]])) {
            _indices[dst++] = _indices[ii];
            oldstring = &_names[_indices[ii]];
        }
    }
    _indices.erase(_indices.begin() + dst, _indices.end());
}

const char *NamesList::GetName(uint32_t index) const
{
    if (index >= _indices.size()) return(NULL);
    return(&_names[_indices[index]]);
}

int NamesList::SortedInsert(const char *name)
{
    int         position, ii;
    const char  *scan;

    // make room in the indices vector
    position = BinarySearchNearestHigher(name);
    if (position == -1) {
        // all elements are lower.
        position = _indices.size();
    } else if (stricmp(name, &_names[_indices[position]]) == 0) {
        // avoid duplication
        return(position);
    }
    _indices.push_back(0);
    for (ii = _indices.size() - 1; ii > position; --ii) {
        _indices[ii] = _indices[ii - 1];
    }

    // save the string
    _indices[position] = _names.size();
    for (scan = name; *scan; ++scan) {
        _names.push_back(*scan);
    }
    _names.push_back(0);
    return(position);
}

int NamesList::BinarySearchNearestLower(const char *name)
{
    int         first, last, mid, cmp;

    first = 0;
    last = _indices.size() - 1;
    if (last < 0 || stricmp(name, &_names[_indices[0]]) < 0) {
        return(-1);
    }
    while (first != last) {
        mid = (first + last + 1) >> 1;
        cmp = stricmp(name, &_names[_indices[mid]]);
        if (cmp == 0) return(mid);
        if (cmp > 0) {
            first = mid;
        } else {
            last = mid - 1;
        }
    }
    return(first);
}

int NamesList::BinarySearchNearestHigher(const char *name)
{
    int         first, last, mid, cmp;

    first = 0;
    last = _indices.size() - 1;
    if (last < 0 || stricmp(name, &_names[_indices[last]]) > 0) {
        return(-1);
    }
    while (first != last) {
        mid = (first + last) >> 1;
        cmp = stricmp(name, &_names[_indices[mid]]);
        if (cmp == 0) return(mid);
        if (cmp > 0) {
            first = mid + 1;
        } else {
            last = mid;
        }
    }
    return(first);
}

int NamesList::BinarySearch(const char *name)
{
    int index;

    index = BinarySearchNearestLower(name);
    if (index != -1 && stricmp(name, &_names[_indices[index]]) == 0) {
        return(index);
    }
    return(-1);
}

int NamesList::BinarySearchRangeWithPrefix(int &index, const char *prefix)
{
    string  next;
    int     top, last_char;

    // get the interval begin. the interval begins with the prefix (if present) or
    // with the following items.
    // (longer strings with same beginning have an higher rank and appear soon after)
    index = BinarySearchNearestHigher(prefix);

    // no element is big enough to have this prefix.
    if (index == -1) return(0);

    // Where do items begin wich start with the next possible prefix in alpha order ?
    next = prefix;
    last_char = next.length() - 1;
    next.setchar(last_char, prefix[last_char] + 1);
    top = BinarySearchNearestHigher(next.c_str());

    // if not found our rage has not an upper bound (top is actually the end of the list)
    if (top == -1) top = _indices.size();

    // done.
    return(top - index);
}

int  NamesList::LinearSearch(const char *name)
{
    int         ii;
    const char *tocompare;

    for (ii = 0; ii < (int)_indices.size(); ++ii) {
        tocompare = &_names[_indices[ii]];
        if (*tocompare == name[0]) {
            if (strcmp(tocompare, name) == 0) {
                return(ii);
            }
        }
    }
    return(-1);
}

void NamesList::TerminateAndCreateIndices(void)
{
    int count, ii;

    // ensure termination
    count = _names.size();
    if (_names[count - 1] != 0) {
        _names.push_back(0);
    }

    // write down the indices
    _indices.push_back(0);
    for (ii = 1; ii < count; ++ii) {
        if (_names[ii - 1] == 0) _indices.push_back(ii);
    }
}

bool NamesList::IsEqualTo(NamesList *other)
{
    if (_indices.size() != other->_indices.size() || _names.size() != other->_names.size()) {
        return(false);
    }
    return (_indices.isequal(other->_indices) && _names.isequal(other->_names));
}

void NamesList::CopyFrom(const NamesList *other)
{
    _names = other->_names;
    _indices = other->_indices;
}

}