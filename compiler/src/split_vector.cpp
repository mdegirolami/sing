#include <string.h>
#include "split_vector.h"

namespace SingNames {

SplitVector::SplitVector()
{
    gap_pos_ = gap_width_ = 0;
}

char *SplitVector::getBufferForLoad(int length)
{
    lines_.clear();
    lines_.reserve(1000);
    lines_.push_back(0);    // beginning of first line
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

void SplitVector::patch(int from_row, int from_col, int to_row, int to_col, int allocate, const char *newtext)
{
    if (allocate > gap_width_) {
        widenGap(allocate - gap_width_);
    }
    patch(rowCol2Offset(from_row, from_col), rowCol2Offset(to_row, to_col), newtext);
}

void SplitVector::insert(const char *newtext)
{
    int togrow = strlen(newtext) - gap_width_;
    if (togrow > 0) togrow += 1024;
    widenGap(togrow);
    insertInGap(newtext);
}

void SplitVector::GetLine(string *line, int row)
{
    int start = rowCol2Offset(row, 0);
    int end = rowCol2Offset(row + 1, 0);
    *line = "";
    while (start < end) {
        if (start < gap_pos_) {
            *line += buffer_[start];
        } else {
            *line += buffer_[start + gap_width_];            
        }
        ++start;
    }
}

int  SplitVector::rowCol2Offset(int row, int col)
{
    // where to start ? how many to skip ?
    if (row < 0) row = 0;
    if (col < 0) col = 0;

    int scan;
    if (row < lines_.size()) {
        scan = lines_[row];
    } else {
        scan = gap_pos_ + gap_width_;
        for (int to_skip = row - lines_.size() + 1; to_skip > 0; --to_skip) {
            while (buffer_[scan] != 10 && scan < buffer_.size()) {
                ++scan;
            }
            if (scan == buffer_.size()) {
                return(scan - gap_width_);
            }
            ++scan;
        }
    }

    // skip cols
    while (scan < buffer_.size() && col > 0) {
        if (scan == gap_pos_) scan += gap_width_;
        char cc = buffer_[scan];
        if (cc == 10 || cc == 13) {
            break;
        }
        if ((cc & 0xf8) == 0xf0) {
            col -= 2;       // takes two utf16 cols !!
        } else if ((cc & 0xc0) != 0x80) {
            col -= 1;
        }
        ++scan;
    }

    // skip unicode trailing bytes
    while (scan < buffer_.size() && (buffer_[scan] & 0xc0) == 0x80) {
        ++scan;
    }

    return(scan > gap_pos_ ? scan - gap_width_ : scan);
}

int SplitVector::offset2VsCol(const char *row, int offset)
{
    int count = 0;
    for (int ii = 0; ii < offset; ++ii) {
        int cc = row[ii];
        if (cc == 0) return(count);
        if ((cc & 0xf8) == 0xf0) {
            count += 2;       // takes two utf16 cols !!
        } else if ((cc & 0xc0) != 0x80) {
            count += 1;
        }
    }
    return(count);
}

int SplitVector::offset2SingCol(const char *row, int offset)
{
    int count = 0;
    for (int ii = 0; ii < offset; ++ii) {
        int cc = row[ii];
        if (cc == 0) return(count);
        if ((cc & 0xc0) != 0x80) {
            count += 1;
        }
    }
    return(count);
}

int SplitVector::VsCol2Offset(const char *row, int col)
{
    const char *scan = row;
    while (*scan != 0 && col > 0) {
        char cc = *scan;
        if ((cc & 0xf8) == 0xf0) {
            col -= 2;       // takes two utf16 cols !!
        } else if ((cc & 0xc0) != 0x80) {
            col -= 1;
        }
        ++scan;
    }
    while ((*scan & 0xc0) == 0x80) {
        ++scan;
    }
    return(scan - row);
}

// int SplitVector::SingCol2offset(const char *row, int col)
// {

// }

void SplitVector::patch(int from, int to, const char *newtext)
{
    if (to < from) to = from;
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
            char cc = buffer_[gap_pos_ + gap_width_];
            buffer_[gap_pos_++] = cc;
            if (cc == 10) {
                lines_.push_back(gap_pos_);
            }
        }
    } else {
        if (new_position < 0) new_position = 0;
        while (gap_pos_ > new_position) {
            --gap_pos_;
            char cc = buffer_[gap_pos_];
            buffer_[gap_pos_ + gap_width_] = cc;
            if (cc == 10) {
                lines_.pop_back();
            }
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
    for (int ii = 0; ii < len; ++ii) {
        char cc = text[ii];
        buffer_[gap_pos_++] = cc; 
        if (cc == 10) {
            lines_.push_back(gap_pos_);
        }
    }
    gap_width_ -= len;
}

} // namespace