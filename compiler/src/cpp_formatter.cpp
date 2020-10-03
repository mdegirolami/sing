#include "cpp_formatter.h"
#include "helpers.h"

namespace SingNames {

CppFormatter::CppFormatter()
{
    max_line_len_ = 160;
    remarks_tabs_ = 20;
    node_pos_ = nullptr;
    Reset();
}

void CppFormatter::Reset(void)
{
    remarks_ = nullptr;
    remarks_count_ = 0;
    line_num_ = 1;
    place_line_break_ = false;
    linenum_map.clear();
}

void CppFormatter::SetRemarks(RemarkDescriptor **remarks, int count)
{
    remarks_ = remarks;
    remarks_count_ = count;
}

void CppFormatter::SetNodePos(IAstNode *node, bool statement) 
{ 
    if (node == nullptr || !node->IsARemarkableNode()) {
        node_pos_ = nullptr;
    } else {
        node_pos_ = node->GetPositionRecord(); 
    }
    formatting_a_statement_ = statement; 
}

//
// pos is the position of the nodes who participate in source
//
void CppFormatter::Format(string *source, int indent)
{
    int minsplit, maxsplit, first_cpp_line;

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
        first_cpp_line = AddComments(&pool_[1], &pool_[0], indent);
        out_str_ = 1;
    } else {
        first_cpp_line = line_num_;
        AppendLf(&pool_[0]);
    }

    // if requested by the synthesizer (may happen between functions or between blocks of different declarations)
    // and not yet present, add a starting empty line
    if (place_line_break_ && pool_[out_str_][0] != '\r') {
        pool_[out_str_].insert(0, "\r\n");
        ++first_cpp_line;
    }
    place_line_break_ = false;

    // if was a statement save a record in linenum_map
    if (formatting_a_statement_) {
        formatting_a_statement_ = false;
        if (node_pos_ != nullptr) {
            line_nums record = { node_pos_->start_row, first_cpp_line};
            linenum_map.push_back(record);
        }
    }
    node_pos_ = nullptr;

    UpdateLineNum();
}

void CppFormatter::UpdateLineNum(void) 
{ 
    const char *retstr = pool_[out_str_].c_str();
    for (const char *scan = retstr; *scan != 0; ++scan) {
        if (*scan == '\n') ++line_num_;
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

void CppFormatter::LookForComments(int baseline, int *side, int *firstidx, int *lastidx, bool *prepend_blank)
{
    *side = *firstidx = *lastidx = -1;
    *prepend_blank = false;

    if (remarks_ == nullptr || remarks_count_ == 0 || baseline < 0) {
        return;
    }

    // returns the comment or the first past it
    int first = 0;
    int last = remarks_count_;
    while (first < last) {
        int mid = (first + last) >> 1;
        int rdline = remarks_[mid]->row;
        if (baseline > rdline) {
            first = mid + 1;
        } else if (baseline < rdline) {
            last = mid;
        } else {
            first = last = mid;
        }
    }

    // perfect match ?
    if (last < remarks_count_) {
        RemarkDescriptor *rd = remarks_[last];
        if (rd->row == baseline) {
            if (rd->rd_type == RdType::COMMENT_AFTER_CODE) {
                *side = last;
            }
        }
    }

    // how many consecutine comments and blank lines compose the comment block ? 
    // (excluding other code in COMMENT_AFTER_CODE lines)
    int to_match = baseline - 1;
    while (first > 0 && remarks_[first - 1]->row == to_match && 
                        remarks_[first - 1]->rd_type != RdType::COMMENT_AFTER_CODE) {
        --first;
        --to_match;    
    }

    // skip leading blank lines
    while (first < last && remarks_[first]->rd_type == RdType::EMPTY_LINE) {
        ++first;
        *prepend_blank = true;
    }    

    *firstidx = first;
    *lastidx = last;
}

int CppFormatter::AddComments(string *formatted, string *source, int indent)
{
    int     sideidx, firstidx, lastidx;
    bool    prepend_blank;
    int     baseline = node_pos_ != nullptr ? node_pos_->start_row : -1;
    int     rem_pos = 0;

    LookForComments(baseline, &sideidx, &firstidx, &lastidx, &prepend_blank);
    
    if (prepend_blank) {
        *formatted = "\r\n";
    } else {
        *formatted = "";
    }

    // prepend the comments
    for (int ii = firstidx; ii < lastidx; ++ii) {
        RemarkDescriptor *rem = remarks_[ii];
        if (rem->rd_type == RdType::COMMENT_ONLY) {
            if (rem->col != 0) {
                AddIndent(formatted, indent);
            }
            *formatted += rem->remark;
        }
        AppendEmptyLine(formatted);
    }

    int retvalue = line_num_;   // line on which we place the first nonremark nonblank code.
    for (const char *scan = formatted->c_str(); *scan != 0; ++scan) {
        if (*scan == '\n') ++retvalue;
    }

    // add the code, split it if too long !
    const char *src_scan = source->c_str();
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
        if (sideidx >= 0) {
            RemarkDescriptor *rem = remarks_[sideidx];
            if (rem->rd_type == RdType::COMMENT_AFTER_CODE) {
                int rem_pos = Maxlen(source);
                if (rem_pos < col + 4) rem_pos = col + 4;
                rem_pos = (rem_pos + remarks_tabs_ - 1) / remarks_tabs_ * remarks_tabs_;
                for (int ii = rem_pos - col; ii > 0; --ii) {
                    *formatted += " ";
                }
                *formatted += rem->remark;
            }
            sideidx = -1;   // don't repeat o next lines !!
        }

        // close the line
        AppendLf(formatted);
    }
    return(retvalue);
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