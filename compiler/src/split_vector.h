#ifndef SPLIT_VECTOR_H
#define SPLIT_VECTOR_H

#include "vector.h"

namespace SingNames {

class SplitVector {
public:
    SplitVector();
    char *getBufferForLoad(int length);
    const char *getAsString();
    void patch(int from_row, int from_col, int to_row, int to_col, int allocate, const char *newtext);
    void insert(const char *newtext);   // at split point !!

private:
    vector<char>    buffer_;
    vector<int>     lines_; 
    int             gap_pos_;
    int             gap_width_;

    // lines_ manipulation
    int  rowCol2Offset(int row, int col);

    void patch(int from, int to, const char *newtext);
    int  actualLen(void);
    void widenGap(int amount);
    void closeGap(void);
    void moveGap(int new_position);
    void deletePastGap(int count);
    void insertInGap(const char *text);
};

}  // namespace

#endif