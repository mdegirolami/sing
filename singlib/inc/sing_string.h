#ifndef SING_STRING_H_
#define SING_STRING_H_

#include <stdint.h>
#include <string.h>
#include "sing_vectors.h"

namespace sing {

class string {
    char    *content_;
    int32_t allocated_;

    // string_bytes does NOT include the terminator
    void growTo(int32_t string_bytes)
    {
        int32_t	newsize;
        char	*newone;

        // NOTE: allocated_ must be strictly > string_bytes to include the terminator
        if (allocated_ > string_bytes) return;
        newsize = allocated_;
        if (newsize == 0) newsize = 1;
        while (newsize <= string_bytes) newsize <<= 1;

        newone = new char[newsize];
        //ASSERT(newone != nullptr);

        strcpy(newone, content_);
        if (allocated_ != 0) {
            delete[] content_;
        }
        content_ = newone;
        allocated_ = newsize;
    }

    // string_bytes does NOT include the terminator
    // same as growTo but doesn't copy the string back.
    void discardAndgrow(int32_t string_bytes)
    {
        int32_t	newsize;
        char	*newone;

        // NOTE: allocated_ must be strictly > string_bytes to include the terminator
        if (allocated_ > string_bytes) return;
        newsize = allocated_;
        if (newsize == 0) newsize = 1;
        while (newsize <= string_bytes) newsize <<= 1;

        newone = new char[newsize];
        //ASSERT(newone != nullptr);

        if (allocated_ != 0) {
            delete[] content_;
        }
        content_ = newone;
        allocated_ = newsize;
    }

    int32_t size(void) const { return(strlen(content_)); }
    static void    rune_encode(char **dst, int32_t rune);   // note: dst is stepped forward
    static int32_t rune_decode(const char **src);           // note: src is stepped forward
    static int32_t rune_to_buffer(char *buffer, int32_t rune) {
        char *dst = buffer;  
        rune_encode(&dst, rune); 
        return(dst - buffer);
    }
    static const char* next_rune_pos(const char *src);      // bytes to the next 
    void move(char *src, int delta);

public:
    static const int32_t npos = -1;

    ~string() { if (allocated_ > 0) delete[] content_; }
    string() : content_((char*)""), allocated_(0) {}
    string(const string &other) : content_(nullptr), allocated_(0) { *this = other; }
    string(const string *other) : content_(nullptr), allocated_(0) { *this = other; }
    string(string &&other) : content_(other.content_), allocated_(other.allocated_) { other.allocated_ = 0; other.content_ = (char*)""; }
    string(const char *c_str) : content_((char*)c_str), allocated_(0) {}    // to init with a literal string

    const char *c_str(void) const { return(&content_[0]); }
    const char *data(void) const { return(&content_[0]); }
    int32_t length(void) const; // number of runes
    void reserve(int32_t size) { growTo(size); }

    // void version avoids using = instead of == by mistake
    void    operator=(const string &other);
    void    operator=(const char *other);       // to assign with a literal string
    void    operator=(string &&other) { content_ = other.content_; allocated_ = other.allocated_; other.allocated_ = 0; other.content_ = (char*)""; }

    string& operator+=(const string& other);
    string& operator+=(const char *cstr);
    string& operator+=(uint32_t rune);

    // all return the next char's position
    int32_t get_rune(int32_t position, int32_t *rune) const;
    int32_t overwrite_rune(int32_t position, int32_t rune);
    int32_t insert_rune(int32_t position, int32_t rune);  

    int32_t pos2charidx(int32_t position) const;
    int32_t charidx2pos(int32_t char_index) const;

    string& erase_from_pos(int32_t pos = 0, int32_t n = npos);
    string& erase_from_idx(int32_t char_idx = 0, int32_t n = npos);

    void    insert_from_pos(int position, const string &toinsert) { insert_from_pos(position, toinsert.content_); };
    void    insert_from_pos(int position, const char *toinsert);
            
    void    insert_from_idx(int idx, const string &toinsert) { insert_from_pos(charidx2pos(idx), toinsert); }
    void    insert_from_idx(int idx, const char *toinsert) { insert_from_pos(charidx2pos(idx), toinsert); };

    int32_t find_pos(int32_t from, int32_t rune) const;
    int32_t rfind_pos(int32_t from, int32_t rune) const;
    int32_t find_idx(int32_t from, int32_t rune) const;
    int32_t rfind_idx(int32_t from, int32_t rune) const;

    string substr_from_pos(int32_t first_pos, int32_t last_pos) const;
    string substr_from_idx(int32_t first_idx, int32_t last_idx) const;

    //const char& operator[] (int32_t pos) const { return(content_[pos]); }
    //char&   operator[] ( int32_t pos )       {return(content_[pos]);}     // dangerous, use setchar (must know if writing a null)
    //void        setchar(int32_t pos, int cc);

    //void    utf8_decode(vector<int32_t> *dst) const;
    //void    utf8_encode(const int32_t *codepoints, int len);

    friend string operator+(const string& left, const string& right);
    friend string operator+(uint32_t rune, const string& right);
    friend string operator+(const string& left, uint32_t rune);

    friend bool operator==(const string& left, const string& right);
    friend bool operator!=(const string& left, const string& right);
    friend bool operator<(const string& left, const string& right);
    friend bool operator<=(const string& left, const string& right);
    friend bool operator>(const string& left, const string& right);
    friend bool operator>=(const string& left, const string& right);
};

string operator+(const string& left, const string& right);
string operator+(uint32_t rune, const string& right);
string operator+(const string& left, uint32_t rune);

bool operator==(const string& left, const string& right);
bool operator!=(const string& left, const string& right);
bool operator<(const string& left, const string& right);
bool operator<=(const string& left, const string& right);
bool operator>(const string& left, const string& right);
bool operator>=(const string& left, const string& right);

} // namespace
#endif
