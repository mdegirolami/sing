#include <cstdarg>
#include <string.h>
#include <float.h>
#include "sing.h"
#include "limits.h"

namespace sing {

const float f32_min = FLT_MIN;
const float f32_eps = FLT_EPSILON;
const float f32_max = FLT_MAX;

const double f64_min = DBL_MIN;
const double f64_eps = DBL_EPSILON;
const double f64_max = DBL_MAX;

const double pi32 = 3.1415926535897932384626433832795f;
const double pi64 = 3.1415926535897932384626433832795;

int64_t string2int(const char *instring)
{
    while (isblank(*instring)) ++instring;
    if (*instring == '0' && instring[1] == 'x') {
        return(strtol(instring + 2, nullptr, 16));
    }
    return(strtol(instring, nullptr, 10));
}

uint64_t string2uint(const char *instring)
{
    while (isblank(*instring)) ++instring;
    if (*instring == '0' && instring[1] == 'x') {
        return(strtoul(instring + 2, nullptr, 16));
    }
    return(strtoul(instring, nullptr, 10));
}

double string2double(const char *instring)
{
    return(strtod(instring, nullptr));
}

std::complex<float> string2complex64(const char *instring)
{
    char    *end;
    float   img;
    float   real = (float)strtod(instring, &end);
    if (*end == 'i' || *end == 'I') {
        img = real;
        while (*end != 0 && (*end < '0' || *end > '9')) ++end;
        real = (float)strtod(end, nullptr);
    } else {
        while (*end != 0 && (*end < '0' || *end > '9')) ++end;
        img = (float)strtod(end, nullptr);
    }
    return(std::complex<float>(real, img));
}

std::complex<double> string2complex128(const char *instring)
{
    char    *end;
    double  img;
    double  real = strtod(instring, &end);
    if (*end == 'i' || *end == 'I') {
        img = real;
        while (*end != 0 && (*end < '0' || *end > '9')) ++end;
        real = strtod(end, nullptr);
    } else {
        while (*end != 0 && (*end < '0' || *end > '9')) ++end;
        img = strtod(end, nullptr);
    }
    return(std::complex<double>(real, img));
}

static const int buf_len = 64;

std::string to_string(std::complex<float> value)
{
    char buffer[buf_len];
    std::string  result;

    snprintf(buffer, buf_len, "%f + %fi", value.real(), value.imag());
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

std::string to_string(std::complex<double> value)
{
    char buffer[buf_len];
    std::string  result;

    snprintf(buffer, buf_len, "%f + %fi", value.real(), value.imag());
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

std::string to_string(bool value)
{
    return(std::string(value ? "true" : "false"));
}

std::string s_format(const char *fmt, ...) {
    int size = (strlen(fmt) << 1) + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}

int32_t hash_from_bytes(const uint8_t *buffer, int count)
{
    int32_t acc = 0x811c9dc5;
    for (; count > 0; --count) {
        acc = (acc * 0x01000193) ^ *buffer++;
    }
    return(acc);
}

} // namespace