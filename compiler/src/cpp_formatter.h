#ifndef CPP_FORMATTER_H
#define CPP_FORMATTER_H

#include "string"
#include "ast_nodes.h"

namespace SingNames {

struct line_nums {
    int sing_line;
    int cpp_line;
};

class CppFormatter {

    // settings
    RemarkDescriptor    **remarks_;
    int                 remarks_count_;
    int                 processed_remarks_;
    PositionInfo        node_pos_;
    int                 max_line_len_;
    int                 remarks_tabs_;   // tab stops for remarks

    // working buffers
    string              pool_[2];
    int                 out_str_;
    vector<line_nums>   linenum_map;

    // status
    bool                place_line_break_;
    int                 line_num_;          // 1-based
    bool                formatting_a_statement_;

    void GetMaxAndMinSplit(string *source, int *minsplit, int *maxsplit);
    void Split(string *formatted, string *source, int minsplit, int maxsplit, int indent);
    void SplitOnFF(string *formatted, string *source, int indent);
    int  Maxlen(string *formatted);
    void AddIndent(string *out_str, int indent);
    void FilterSplitMarkers(string *out_str, string *in_str);
    int  AddComments(string *formatted, string *source);
    void AppendLf(string *formatted);           // adds '\r\n' for the purpose of closing a not empty line
    void AppendEmptyLine(string *formatted);    // adds '\r\n' for the purpose of adding an empty line
    void UpdateLineNum(void);
public:
    CppFormatter();

    // settings
    void Reset(void);
    void SetMaxLineLen(int len) { max_line_len_ = len; }
    void SetRemarks(RemarkDescriptor **remarks, int count);
    void SetNodePos(PositionInfo *pos, bool statement = false) { node_pos_ = *pos; formatting_a_statement_ = statement; }

    // operations
    void Format(string *source, int indent);
    void FormatResidualRemarks(void);
    const char *GetString(void) { return(pool_[out_str_].c_str()); }
    int  GetLength(void) { return(pool_[out_str_].length()); }
    void AddLineBreak(void) { place_line_break_ = true; }
    const vector<line_nums> *GetLines(void) { return(&linenum_map); }
};

} // namespace

#endif
