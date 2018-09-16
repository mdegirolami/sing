#include <string.h>
#include <ctype.h>
#include "string"

namespace StayNames {

string::string(const char *c_str)
{
    _content.insert_range(0, (URGH_SIZE_T)strlen(c_str) + 1, c_str);   // include terminator  
}

string string::operator+(const string& other) const 
{
    string result(this);
    result += other;
    return result; 
}

string string::operator+(const char *cstr) const
{
    string result(this);
    result += cstr;
    return result;
}

string string::operator+(int cc) const
{
    string result(this);
    result += cc;
    return result;
}

string& string::operator+=(const string& other)
{
    _content.insert_range(_content.size() - 1,             // just before the terminator
                          other._content.size() - 1,      // all but terminator
                          &other._content[0]);
    return(*this);
}

string& string::operator+=(const char *cstr)
{
    _content.insert_range(_content.size() - 1,             // just before the terminator
        (URGH_SIZE_T)strlen(cstr), cstr);
    return(*this);
}

string& string::operator+=(int cc)
{
    _content.insert(_content.size() - 1, cc);            // just before the terminator
    return(*this);
}
/*
string& string::operator=(const string &other)
{
    _content = other._content;
    return(*this);
}

string& string::operator=(const char *cstr)
{
    _content.clear();
    _content.insert_range(0, (URGH_SIZE_T)strlen(cstr) + 1, cstr);   // include terminator  
    return(*this);
}
*/

// void version avoids using = instead of == by mistake

void string::operator=(const string &other)
{
    _content = other._content;
}

void string::operator=(const char *cstr)
{
    _content.clear();
    if (cstr != NULL) {
        _content.insert_range(0, (URGH_SIZE_T)strlen(cstr) + 1, cstr);   // include terminator  
    } else {
        _content.push_back(0);
    }
}

string& string::erase(URGH_SIZE_T pos, URGH_SIZE_T n)
{
    URGH_SIZE_T toerase = _content.size() - pos - 1;   
    
    if (n != npos && toerase > n) toerase = n;
    if (toerase > 0) {
        _content.erase(pos, pos + toerase);
    }
    return(*this);
}

bool string::operator==(const string &other) const
{
    return(strcmp(&_content[0], &other._content[0]) == 0);
}

bool string::operator==(const char *cstr) const
{
    return(strcmp(&_content[0], cstr) == 0);
}

bool string::operator!=(const string &other) const
{
    return(strcmp(&_content[0], &other._content[0]) != 0);
}

bool string::operator!=(const char *cstr) const
{
    return(strcmp(&_content[0], cstr) != 0);
}

void string::setchar(URGH_SIZE_T pos, int cc)
{
    // out of bounds ?
    if (pos >= _content.size() - 1) {
        return;
    }

    // truncates the string ?
    if (cc == 0) {
        erase(pos);
        return;
    }

    // assign
    _content[pos] = (char)cc;
}

URGH_SIZE_T string::find(char cc) const
{
    int count = _content.size() - 1;

    for (int ii = 0; ii < count; ++ii) {
        if (_content[ii] == cc) {
            return(ii);
        }
    }
    return(npos);
}

URGH_SIZE_T  string::rfind(char cc) const
{
    int scan;

    for (scan = _content.size() - 2; scan >= 0; --scan) {
        if (_content[scan] == cc) return(scan);
    }
    return(-1);
}

string string::substr(URGH_SIZE_T first, URGH_SIZE_T last) const
{
    string result;
    if (first < 0) first = 0;
    if (last > _content.size() - 1) last = _content.size() - 1;
    if (last > first) { 
        result._content.insert_range(0, last - first, &_content[first]);
    }
    return(result);
}

void string::replace(const char tofind, const char towrite)
{
    int count = _content.size() - 1;

    for (int ii = 0; ii < count; ++ii) {
        if (_content[ii] == tofind) {
            _content[ii] = towrite;
        }
    }
}

void string::toupper(void)
{
    int count = _content.size() - 1;

    for (int ii = 0; ii < count; ++ii) {
        _content[ii] = ::toupper(_content[ii]);
    }
}

URGH_SIZE_T string::find_from_nocase(URGH_SIZE_T first_pos, char cc) const
{
    int count = _content.size() - 1;

    for (int ii = first_pos; ii < count; ++ii) {
        if (::toupper(_content[ii]) == ::toupper(cc)) {
            return(ii);
        }
    }
    return(npos);
}

URGH_SIZE_T string::string_in_string_nocase(const string &tofind) const
{
    int scan = 0;
    int first_char;

    first_char = tofind[0];
    if (first_char == 0) return(npos);
    do {
        // optimization: search first char first
        scan = find_from_nocase(scan, first_char);
        if (scan == npos) {
            return(npos);
        }
    } while (stricmp(&_content[scan + 1], &tofind._content[1]) != 0);
    return(scan);
}

void string::utf8_decode(vector<int32_t> *dst) const
{
    int         chars_left;
    const char  *scan;

    dst->reserve(dst->size() + _content.size() - 1);
    chars_left = _content.size();
    scan = &_content[0];
    while (chars_left > 0) {
        if ((*scan & 0x80) == 0) {
            dst->push_back(*scan++);
            --chars_left;
        } else if ((*scan & 0xe0) == 0xc0) {
            if (chars_left < 2 || (scan[1] & 0xc0) != 0x80) {
                ++scan;
                --chars_left;
            } else {
                dst->push_back(((scan[0] & 0x1f) << 6) | (scan[1] & 0x3f));
                scan += 2;
                chars_left -= 2;
            }
        } else if ((*scan & 0xf0) == 0xe0) {
            if (chars_left < 3 || (scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80) {
                ++scan;
                --chars_left;
            } else {
                dst->push_back(((scan[0] & 0xf) << 12) | ((scan[1] & 0x3f) << 6) | (scan[2] & 0x3f));
                scan += 3;
                chars_left -= 3;
            }
        } else if ((*scan & 0xf8) == 0xf0) {
            if (chars_left < 4 || (scan[1] & 0xc0) != 0x80 || (scan[2] & 0xc0) != 0x80 || (scan[3] & 0xc0) != 0x80) {
                ++scan;
                --chars_left;
            } else {
                dst->push_back(((scan[0] & 0x7) << 18) | ((scan[1] & 0x3f) << 12) | ((scan[2] & 0x3f) << 6) | (scan[3] & 0x3f));
                scan += 4;
                chars_left -= 4;
            }
        } else {
            // illegal !, just skip.
            ++scan;
            --chars_left;
        }
    }
}

void string::utf8_encode(const int32_t *codepoints, int len)
{
    int     ii;
    int32_t cp;

    // take the terminator out of the way
    _content.pop_back();    

    // append the codepoints
    for (ii = 0; ii < len; ++ii) {
        cp = codepoints[ii];
        if (cp < 0x80) {
            _content.push_back(cp);
        } else if (cp < 0x800) {
            _content.push_back((cp >> 6) | 0xc0);
            _content.push_back((cp & 0x3f) | 0x80);
        } else if (cp < 0x10000) {
            _content.push_back((cp >> 12) | 0xe0);
            _content.push_back(((cp >> 6) & 0x3f) | 0x80);
            _content.push_back((cp & 0x3f) | 0x80);
        } else {
            _content.push_back(((cp >> 18) & 7) | 0xf0);
            _content.push_back(((cp >> 12) & 0x3f) | 0x80);
            _content.push_back(((cp >> 6) & 0x3f) | 0x80);
            _content.push_back((cp & 0x3f) | 0x80);
        }
    }

    // restore the terminator
    if (_content[_content.size() - 1] != 0) {
        _content.push_back(0);
    }
}

}
