#include <string.h>
#include "helpers.h"

namespace SingNames {

void ErrorList::AddError(const char *message, int nRow, int nCol, int nEndRow, int nEndCol)
{
    errors_strings_.AddName(message);
    rows_.push_back(nRow);
    cols_.push_back(nCol);
    end_rows_.push_back(nEndRow);
    end_cols_.push_back(nEndCol);    
}

const char *ErrorList::GetError(int index, int *nRow, int *nCol, int *nEndRow, int *nEndCol) const
{
    if (index < 0 || index >= (int)rows_.size()) {
        return(nullptr);
    }
    *nRow = rows_[index];
    *nCol = cols_[index];
    if (nEndRow != nullptr) *nEndRow = end_rows_[index];
    if (nEndCol != nullptr) *nEndCol = end_cols_[index];
    return(errors_strings_.GetName(index));
}

void ErrorList::Reset(void)
{
    rows_.clear();
    cols_.clear();
    end_rows_.clear();
    end_cols_.clear();
    errors_strings_.Reset();
}

void ErrorList::Append(const ErrorList *source)
{
    errors_strings_.CopyFrom(&source->errors_strings_);
    rows_.append(source->rows_);
    cols_.append(source->cols_);
    end_rows_.append(source->end_rows_);
    end_cols_.append(source->end_cols_);
}

static int stub(int a, int b, void *context)
{
    return(((ErrorList*)context)->CompareForSort(a, b));
}

void ErrorList::Sort(void)
{
    int         *indices;
    ErrorList   sorted;

    int nitems = (int)rows_.size();

    // build indices
    if (nitems < 2) return;
    indices = new int[nitems];
    for (int ii = 0; ii < nitems; ++ii) indices[ii] = ii;

    // sort indices
    quick_sort_indices(indices, nitems, stub, this);

    // reorder messages
    for (int ii = 0; ii < nitems; ++ii)
    {
        int actual = indices[ii];
        const char *message = errors_strings_.GetName(actual);
        sorted.AddError(message, rows_[actual], cols_[actual], end_rows_[actual], end_cols_[actual]);
    }
    Reset();
    Append(&sorted);

    // free used-up resources
    delete[] indices;
}

int ErrorList::CompareForSort(int a, int b)
{ 
    if (rows_[a] != rows_[b]) {
        return(rows_[a] - rows_[b]);
    }
    if (cols_[a] != cols_[b]) {
        return(cols_[a] - cols_[b]);
    }
    return(strcmp(errors_strings_.GetName(a), errors_strings_.GetName(b))); 
}

void quick_sort_indices(int *vv, int count, int(*comp)(int, int, void *), void *context)
{
    int tmp, lower, upper, pivot;

    // trivial cases
    if (count < 2) return;
    if (count == 2) {
        if (comp(vv[0], vv[1], context) > 0) {
            tmp = vv[1];
            vv[1] = vv[0];
            vv[0] = tmp;
        }
        return;
    }

    // sort around the pivot
    lower = 0;
    upper = count - 1;
    pivot = count >> 1;
    while (true) {

        // find an item preceeding the pivot that should stay after the pivot.
        while (lower < pivot && comp(vv[lower], vv[pivot], context) <= 0) {
            ++lower;
        }

        // find an item succeeding the pivot that should stay before the pivot.
        while (upper > pivot && comp(vv[upper], vv[pivot], context) >= 0) {
            --upper;
        }

        // swap them
        if (lower < pivot) {
            if (upper > pivot) {
                tmp = vv[lower];
                vv[lower] = vv[upper];
                vv[upper] = tmp;
                ++lower;
                --upper;
            } else {

                // lower is out of place but not upper.
                // move the pivot down one position to make room for lower
                tmp = vv[pivot];
                vv[pivot] = vv[lower];
                vv[lower] = vv[pivot - 1];
                vv[pivot - 1] = tmp;
                --pivot;
                upper = pivot;
            }
        } else {
            if (upper > pivot) {

                // upper is out of place but not lower.
                tmp = vv[pivot];
                vv[pivot] = vv[upper];
                vv[upper] = vv[pivot + 1];
                vv[pivot + 1] = tmp;
                ++pivot;
                lower = pivot;
            } else {
                break;
            }
        }
    }

    // recur
    if (pivot > 1) {
        quick_sort_indices(vv, pivot, comp, context);
    }
    tmp = count - pivot - 1;
    if (tmp > 1) {
        quick_sort_indices(vv + (pivot + 1), tmp, comp, context);
    }
}

}