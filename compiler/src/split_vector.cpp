#include <string.h>
#include "split_vector.h"

namespace SingNames {

SplitVector::SplitVector()
{
    gap_pos_ = gap_width_ = 0;
}

char *SplitVector::getBufferForLoad(int length)
{
    buffer_.clear();
    buffer_.reserve(length + 1);
    buffer_.resize(length);
    gap_pos_ = gap_width_ = 0;
    return(&buffer_[0]);
}

const char *SplitVector::getAsString()
{
    closeGap();
    int len = buffer_.size();
    if (len < 1 || buffer_[len -1] != 0) {
        buffer_.push_back(0);
    }    
    return(&buffer_[0]);
}

void SplitVector::patch(int from, int to, const char *newtext)
{
    int togrow = strlen(newtext) - (to - from) - gap_width_;
    if (togrow > 0) togrow += 1024;
    if (from < gap_pos_) {
        widenGap(togrow);
        moveGap(from);
    } else {
        moveGap(from);
        widenGap(togrow);
    }
    deletePastGap(to - from);
    insertInGap(newtext);
}

int SplitVector::actualLen(void)
{
    return(buffer_.size() - gap_width_);
}

void SplitVector::widenGap(int amount)
{
    if (amount <= 0) return;

    // enlarge the vector
    int oldlen = buffer_.size();
    buffer_.resize(oldlen + amount);

    // align data past the gap to the end of the buffer 
    char *dst = &buffer_[oldlen + amount - 1];
    char *src = &buffer_[oldlen - 1];
    for (int count = oldlen - gap_pos_ - gap_width_; count > 0; --count) {
        *dst-- = *src--;
    }

    // update the gap
    gap_width_ += amount;
}

void SplitVector::closeGap(void)
{
    buffer_.erase(gap_pos_, gap_pos_ + gap_width_);
    gap_width_ = 0;
}

void SplitVector::moveGap(int new_position)
{
    if (new_position > gap_pos_) {
        int max_pos = actualLen();
        if (new_position > max_pos) new_position = max_pos;
        while (gap_pos_ < new_position) {
            buffer_[gap_pos_] = buffer_[gap_pos_ + gap_width_];
            ++gap_pos_;
        }
    } else {
        if (new_position < 0) new_position = 0;
        while (gap_pos_ > new_position) {
            --gap_pos_;
            buffer_[gap_pos_ + gap_width_] = buffer_[gap_pos_];
        }
    }
}

void SplitVector::deletePastGap(int count)
{
    int max_deletable = buffer_.size() - (gap_pos_ + gap_width_);
    if (count > max_deletable) count = max_deletable;
    gap_width_ += count;
}

void SplitVector::insertInGap(const char *text)
{
    int len = strlen(text);
    if (len > gap_width_) len = gap_width_;
    memcpy(&buffer_[gap_pos_], text, len);
    gap_width_ -= len;
    gap_pos_ += len;
}

} // namespace