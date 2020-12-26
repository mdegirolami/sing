#ifndef SPLIT_VECTOR_H
#define SPLIT_VECTOR_H

#include "vector.h"

namespace SingNames {

class SplitVector {
public:
    SplitVector();
    char *getBufferForLoad(int length);
    const char *getAsString();
    void patch(int from, int to, const char *newtext);

private:
    vector<char>    buffer_;
    int             gap_pos_;
    int             gap_width_;

    int  actualLen(void);
    void widenGap(int amount);
    void closeGap(void);
    void moveGap(int new_position);
    void deletePastGap(int count);
    void insertInGap(const char *text);
};

}  // namespace

#endif