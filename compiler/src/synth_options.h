#ifndef __SYNTH_OPTIONS_H_
#define __SYNTH_OPTIONS_H_

#include "string"

namespace SingNames {

class SynthOptions {
public:
    SynthOptions();
    void validate(void);
    bool load(const char *filename);

    int     max_linelen_;
    bool    newline_before_function_bracket_;
    string  member_prefix_;
    string  member_suffix_;
    bool    use_final_;
    bool    use_override_;
};

} // namespace

#endif
