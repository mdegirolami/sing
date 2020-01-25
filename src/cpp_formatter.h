#ifndef CPP_FORMATTER_H
#define CPP_FORMATTER_H

#include "string"
#include "ast_nodes.h"

namespace SingNames {

class CppFormatter {

    // settings
    RemarkDescriptor    **remarks_;
    int                 remarks_count_;
    int                 processed_remarks_;
    PositionInfo        node_pos;
    int                 max_line_len_;
    int                 remarks_tabs;   // tab stops for remarks
    int                 split_limit_;   // split codes below split_limit_ indent the same as split_limit_

    // working buffers
    string              pool_[2];
    int                 out_str_;

    // status
    bool                place_line_break_;

    void GetMaxAndMinSplit(string *source, int *minsplit, int *maxsplit);
    void Split(string *formatted, string *source, int minsplit, int maxsplit, int indent);
    void SplitOnFF(string *formatted, string *source, int indent);
    int  Maxlen(string *formatted);
    void AddIndent(string *out_str, int indent);
    void FilterSplitMarkers(string *out_str, string *in_str);
    void AddComments(string *formatted, string *source);
    void AppendLf(string *formatted);           // adds '\r\n' for the purpose of closing a not empty line
    void AppendEmptyLine(string *formatted);    // adds '\r\n' for the purpose of adding an empty line
public:
    CppFormatter();

    // settings
    void Reset(void);
    void SetMaxLineLen(int len) { max_line_len_ = len; }
    void SetRemarks(RemarkDescriptor **remarks, int count);
    void SetNodePos(PositionInfo *pos) { node_pos = *pos; }

    // operations
    void Format(string *source, int indent);
    void FormatResidualRemarks(void);
    const char *GetString(void) { return(pool_[out_str_].c_str()); }
    int  GetLength(void) { return(pool_[out_str_].length()); }
    void AddLineBreak(void) { place_line_break_ = true; }

    void LockSplitIndent(int indent) { /*if (split_limit_ != 0xff) split_limit_= indent;*/ }
    void UnLockSplitIndent(void) { split_limit_ = 0xff; }
};

} // namespace

#endif
