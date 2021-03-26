#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "synth_options.h"

namespace SingNames {

// returns true on t, T, 1, else false
static bool stringIsTrue(string &vv) 
{
    return(vv == "t" || vv == "T" || vv == "1");
}

// returns false on f, F, 0 else true
static bool stringNotFalse(string &vv) 
{
    return(vv != "f" && vv != "F" && vv != "0");
}

static void checkShortName(string *vv)
{
    if (vv->length() > 8) {
        *vv = "";
        return;
    }
}

static bool iscrlf(int cc)
{
    return(cc == '\n' || cc == '\r');
}

enum ParserStatus {PS_SKIP, PS_KEY, PS_EQUAL, PS_VALUE, PS_EOL, PS_ERR};

static bool parseALine(string *key, string *value, FILE *fd)
{
    ParserStatus status = PS_SKIP;
    bool valid = false;

    while (true) {
        int cc = getc(fd);
        if (cc == EOF) {
            return (status == PS_VALUE || status == PS_EOL || status == PS_EQUAL);
        }
        if (iscrlf(cc)) {
            if (status == PS_VALUE || status == PS_EOL || status == PS_EQUAL) {
                return(true);
            } else {
                status = PS_SKIP;
            }
            continue;
        }
        switch (status) {
        case PS_SKIP:
            if (isalnum(cc)) {
                status = PS_KEY;
                *key = "";
                *value = "";
                *key += cc;
            } else if (!isblank(cc)) {
                status = PS_ERR;
            }
            break;
        case PS_KEY:
            if (isalnum(cc) || cc == '_') {
                *key += cc;
            } else if (isblank(cc) || cc == '=') {
                status = PS_EQUAL;
            } else {
                status = PS_ERR;
            }
            break;
        case PS_EQUAL:
            if (isalnum(cc) || cc == '_') {
                status = PS_VALUE;
                *value += cc;
            } else if (!isblank(cc) && cc != '=') {
                status = PS_ERR;
            }
            break;
        case PS_VALUE:
            if (isalnum(cc) || cc == '_') {
                *value += cc;
            } else {
                status = PS_EOL;
            }
            break;
        case PS_EOL:
        case PS_ERR:
            break;
        }
    }
}

SynthOptions::SynthOptions()
{
    max_linelen_ = 160;
    newline_before_function_bracket_ = true;
    //member_prefix_ = "";
    member_suffix_ = "_";
    use_final_ = true;
    use_override_ = true;
}

void SynthOptions::validate(void)
{
    if (max_linelen_ < 80) max_linelen_ = 80;
    checkShortName(&member_prefix_);
    checkShortName(&member_suffix_);
}

bool SynthOptions::load(const char *filename)
{
    if (filename == nullptr) return(false);
    FILE *fd = fopen(filename, "rb");
    if (fd == nullptr) return(false);
    string key, value;
    while (parseALine(&key, &value, fd)) {
        if (key == "max_linelen") {
            max_linelen_ = atoi(value.c_str());
        } else if (key == "newline_before_function_bracket") {
            newline_before_function_bracket_ = stringNotFalse(value);
        } else if (key == "member_prefix") {
            member_prefix_ = value;
        } else if (key == "member_suffix") {
            member_suffix_ = value;
        } else if (key == "use_final") {
            use_final_ = stringNotFalse(value);
        } else if (key == "use_override") {
            use_override_ = stringNotFalse(value);
        }
    }
    fclose(fd);
    validate();
    return(true);
}



} // namespace