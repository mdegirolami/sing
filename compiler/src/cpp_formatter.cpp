#include "cpp_formatter.h"
#include "helpers.h"

namespace SingNames {

CppFormatter::CppFormatter()
{
    max_line_len_ = 80;
    remarks_tabs = 20;
    Reset();
}

void CppFormatter::Reset(void)
{
    remarks_ = nullptr;
    remarks_count_ = 0;
    place_line_break_ = false;
}

void CppFormatter::SetRemarks(RemarkDescriptor **remarks, int count)
{
    remarks_ = remarks;
    remarks_count_ = count;
    processed_remarks_ = 0;
}

//
// pos is the position of the nodes who participate in source
//
void CppFormatter::Format(string *source, int indent)
{
    int minsplit, maxsplit;

    // format on a single or multiple lines, with correct indent.
    GetMaxAndMinSplit(source, &minsplit, &maxsplit);
    if (maxsplit == 0xff) {
        //Split(&pool_[0], source, 0xff, 0xff, indent);
        SplitOnFF(&pool_[0], source, indent);
    } else if (maxsplit == 0 || Maxlen(source) + (indent << 2) <= max_line_len_) {
        pool_[0] = "";
        AddIndent(&pool_[0], indent);
        if (maxsplit == 0) {
            pool_[0] += *source;
        } else {
            FilterSplitMarkers(&pool_[0], source);
        }
    } else {
        Split(&pool_[0], source, minsplit, maxsplit, indent);
    }
    out_str_ = 0;

    // add remarks and a terminating \r\n.
    // adds a starting \r\n if there is a user placed empy row just before the code
    if (remarks_ != nullptr && remarks_count_ > 0) {
        AddComments(&pool_[1], &pool_[0]);
        out_str_ = 1;
    } else {
        AppendLf(&pool_[0]);
    }

    // if requested by the synthesizer (may happen between functions or between blocks of different declarations)
    // and not yet present, add a starting empty line
    if (place_line_break_ && pool_[out_str_][0] != '\r') {
        pool_[out_str_].insert(0, "\r\n");
    }
    place_line_break_ = false;
}

void CppFormatter::FormatResidualRemarks(void)
{
    pool_[0] = "";
    out_str_ = 0;
    if (remarks_ == nullptr || remarks_count_ == 0) {
        return;
    }
    while (processed_remarks_ < remarks_count_) {
        RemarkDescriptor *rem = remarks_[processed_remarks_++];
        if (!rem->emptyline) {
            for (int ii = rem->col; ii > 0; --ii) {
                pool_[0] += " ";
            }
            pool_[0] += rem->remark;
        }
        AppendEmptyLine(&pool_[0]);
    }
    if (pool_[0].length() == 0) {
        return;
    } else if (pool_[0].length() < 3) {
        // just a line feed - try to merge with the next
        pool_[0] = 0;
        AddLineBreak();
    } else if (pool_[0][pool_[0].length() - 3] == '\n') {
        // ends with a line feed - try to merge with the next
        pool_[0].erase(pool_[0].length() - 2);
        AddLineBreak();
    }
}

void CppFormatter::GetMaxAndMinSplit(string *source, int *minsplit, int *maxsplit)
{
    int min = 0xff;
    int max = 0;

    for (int ii = 0; ii < (int)source->length(); ++ii) {
        unsigned char val = (*source)[ii];
        if (val >= 0xf8) {
            min = MIN(min, val);
            max = MAX(max, val);
        }
    }
    *minsplit = min;
    *maxsplit = max;
}

#if 0
void CppFormatter::Split(string *formatted, string *source, int minsplit, int maxsplit, int indent)
{
    bool    try_again = true;
    for (int splitter = maxsplit; try_again; --splitter) {
        int     linelen = 0;

        try_again = false;
        *formatted = "";
        AddIndent(formatted, indent);
        linelen = indent << 2;
        for (int ii = 0; ii < (int)source->length(); ++ii) {
            unsigned char val = (*source)[ii];
            if (val >= splitter && val <= maxsplit) {

                // split
                *formatted += "\r\n";

                // indent
                //int full_indent = indent + MAX(maxsplit, split_limit_) - val + 1;
                int full_indent = indent + 1;
                AddIndent(formatted, full_indent);
                linelen = full_indent << 2;
            } else if (val < 0xf8) {
                *formatted += val;

                // do we need a finer grained split ?
                if (splitter != minsplit && (val & 0xc0) != 0x80) {
                    if (++linelen > max_line_len_) {
                        try_again = true;
                        break;
                    }
                }
            }
        }
    }
}

#else

void CppFormatter::Split(string *formatted, string *source, int minsplit, int maxsplit, int indent)
{
    const char *scan = source->c_str();
    int         cur_indent = indent;

    *formatted = "";
    do {
        // get row info
        const char *best_split = nullptr;
        int split_score = 0;
        int linelen = cur_indent << 2;

        // if a split was found stop as you reach max_line_length_ (and use the split)
        // else go on searching to the end of the line.
        const char *rowscan;
        for (rowscan = scan; *rowscan != 0 && *rowscan != '\r' && (linelen < max_line_len_ || best_split == nullptr); ++rowscan) {
            unsigned char val = *rowscan;
            if (val >= 0xf8) {
                int dist = max_line_len_ - linelen;
                if (val >= split_score) {   // >= because the later the better
                    best_split = rowscan;
                    split_score = val;
                }
            } else if ((val & 0xc0) != 0x80) {
                ++linelen;
            }
        }

        // no split point or the line length didn't exceed the limit: copy up to the end of line 
        if (best_split == nullptr || *rowscan == 0 || *rowscan == '\r') best_split = rowscan;

        // discard trailing blanks
        const char *top = best_split;
        while (top > scan && (top[-1] == ' ' || top[-1] == '\t')) {
            --top;
        }

        // copy the line, up to the split point
        AddIndent(formatted, cur_indent);
        for (rowscan = scan; rowscan < top; ++rowscan) {
            unsigned char val = *rowscan;
            if (val < 0xf8) {
                *formatted += val;
            }
        }
        *formatted += "\r\n";

        // skip the split or the line break
        for (scan = best_split; *scan == '\r' || *scan == '\n' || *(unsigned char*)scan >= 0xf8; ++scan);

        // select an indent for the next row (we gave up and just decided for a single indent).
        cur_indent = indent + 1; // ((0xff - split_score > 1) ? 2 : 1);
    } while (*scan != 0);
}

void CppFormatter::SplitOnFF(string *formatted, string *source, int indent)
{
    // copy the line, up to the split point (if any)
    *formatted = "";
    AddIndent(formatted, indent);
    for (const char *scan = source->c_str(); *scan != 0; ++scan) {
        unsigned char val = *scan;
        if (val == 0xff) {
            *formatted += "\r\n";
            if (strcmp(scan + 1, "};") == 0) {
                AddIndent(formatted, indent);   // keep the last closing brace aligned to the declaration
            } else {
                AddIndent(formatted, indent + 1);
            }
        } else if (val == '\r') {
            *formatted += "\r\n";
            AddIndent(formatted, indent + 1);
            if (scan[1] != 0) ++scan;
        } else if (val < 0xf8) {
            *formatted += val;
        }
    }
    *formatted += "\r\n";
}

#endif

// find the longest line
int CppFormatter::Maxlen(string *formatted)
{
    int curline = 0;
    int maxline = 0;
    for (int ii = 0; ii < (int)formatted->length(); ++ii) {
        unsigned char val = (*formatted)[ii];
        if (val >= 0xf8 || (val & 0xc0) == 0x80) continue;
        if (val == '\r' || val == '\n') {
            maxline = MAX(curline, maxline);
            curline = 0;
        } else {
            ++curline;
        }
    } 
    return(MAX(curline, maxline));
}

void CppFormatter::AddIndent(string *out_str, int indent)
{
    for (int ii = indent; ii > 0; --ii) {
        *out_str += "    ";
    }
}

void CppFormatter::FilterSplitMarkers(string *out_str, string *in_str)
{
    for (int ii = 0; ii < (int)in_str->length(); ++ii) {
        unsigned char val = (*in_str)[ii];
        if (val < 0xf8) {
            (*out_str) += val;
        }
    }
}

void CppFormatter::AddComments(string *formatted, string *source)
{
    *formatted = "";
    const char *src_scan = source->c_str();
    while (processed_remarks_ < remarks_count_ && remarks_[processed_remarks_]->row < node_pos.start_row) {
        RemarkDescriptor *rem = remarks_[processed_remarks_++];
        if (!rem->emptyline) {
            for (int ii = rem->col; ii > 0; --ii) {
                *formatted += " ";
            }
            *formatted += rem->remark;
        }
        AppendEmptyLine(formatted);
    }

    int currow = node_pos.start_row;
    int rem_pos = (Maxlen(source) / remarks_tabs + 1) * remarks_tabs;
    while (*src_scan != 0) {

        // copy a line
        int col = 0;
        while (*src_scan != 0 && *src_scan != '\r') {
            unsigned char val = *src_scan++;
            if ((val & 0xc0) != 0x80) {
                ++col;
            }
            *formatted += val;
        }

        // skip the line break
        if (*src_scan == '\r') {
            src_scan += 2;
        }

        // add just one comment, if any
        if (processed_remarks_ < remarks_count_ && remarks_[processed_remarks_]->row <= node_pos.last_row) {
            RemarkDescriptor *rem = remarks_[processed_remarks_];
            if (!rem->emptyline) {
                processed_remarks_++;

                //for (int ii = (col / remarks_tabs + 1) * remarks_tabs - col; ii > 0; --ii) {
                for (int ii = rem_pos - col; ii > 0; --ii) {
                    *formatted += " ";
                }
                *formatted += rem->remark;
            }
        }

        // close the line
        AppendLf(formatted);
    }
}

// don't append if terminating with a line break
void CppFormatter::AppendLf(string *formatted)
{
    // check if already present to avoid duplications
    if (formatted->length() < 2 || (*formatted)[formatted->length() - 1] != '\n') {
        *formatted += "\r\n";
    }
}

// don't append if terminating with two line breaks
void CppFormatter::AppendEmptyLine(string *formatted)
{
    int len = formatted->length();
    if (len < 4 || (*formatted)[len - 3] != '\n' || (*formatted)[len - 1] != '\n') {
        *formatted += "\r\n";
    }
}

} // namespace