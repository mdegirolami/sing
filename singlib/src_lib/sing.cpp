#include <cstdarg>
#include <sing.h>

namespace sing {

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

string tostring(int value)
{
    char    buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%d", value);
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

string tostring(long long value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%lld", value);
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

string tostring(unsigned int value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%u", value);
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

string tostring(unsigned long long value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%llu", value);
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

/*
string tostring(float value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%g", value, 0);
    snprintf(buffer, buf_len, "%g", value, -1);
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}
*/

string tostring(double value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%g", value);
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

string tostring(std::complex<float> value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%g + %gi", value.real(), value.imag());
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

string tostring(std::complex<double> value)
{
    char buffer[buf_len];
    string  result;

    snprintf(buffer, buf_len, "%g + %gi", value.real(), value.imag());
    result += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(result);
}

string tostring(bool value)
{
    return(value ? string("true") : string("false"));
}

string add_strings(int count, ...)
{
    int         string_idx;
    va_list     args;
    string      retval;
    const char  *src;
    char        *dst;
    size_t      total_bytes = 0;

    // find the total required size
    va_start(args, count);
    for (string_idx = 0; string_idx < count; ++string_idx) {
        total_bytes += strlen(va_arg(args, const char *));
    }
    va_end(args);

    // build the buffer
    char *buffer = (char*)alloca(total_bytes + 1);

    va_start(args, count);
    dst = buffer;
    for (string_idx = 0; string_idx < count; ++string_idx) {
        src = va_arg(args, const char *);
        while (*src) {
            *dst++ = *src++;
        }
    }
    va_end(args);
    *dst++ = 0;
    retval += buffer;       // tricky - constructing a string with a char* doesn't allocate the buffer
    return(retval);
}

string format(const char *format, ...)
{
    va_list     args;
    string      retval;
    const char  *scan;
    size_t      total_bytes = 0;
    string      result;
    char        buffer[buf_len];

    // find the total required size
    va_start(args, format);
    for (scan = format; *scan != 0; ++scan) {
        switch (*scan) {
        case 'd':
        case 'u':
            total_bytes += 11;
            va_arg(args, int32_t);
            break;
        case 'D':
        case 'U':
            total_bytes += 21;
            va_arg(args, int64_t);
            break;
        case 'c':
            total_bytes += 4;
            va_arg(args, int32_t);
            break;
        case 'C':
            total_bytes += 4;
            va_arg(args, int64_t);
            break;
        case 'f':
            total_bytes += 15;
            va_arg(args, double);
            break;
        case 's':
            total_bytes += strlen(va_arg(args, const char *));
            break;
        case 'b':
            total_bytes += 5;
            va_arg(args, bool);
            break;
        case 'r':
            total_bytes += 35;
            va_arg(args, std::complex<float>);
            break;
        case 'R':
            total_bytes += 35;
            va_arg(args, std::complex<double>);
            break;
        }
    }
    va_end(args);

    result.reserve(total_bytes + 1);

    va_start(args, format);
    for (scan = format; *scan != 0; ++scan) {
        switch (*scan) {
        case 'd':
            snprintf(buffer, buf_len, "%d", va_arg(args, int32_t));
            result += buffer;
            break;
        case 'D':
            snprintf(buffer, buf_len, "%lld", va_arg(args, int64_t));
            result += buffer;
            break;
        case 'u':
            snprintf(buffer, buf_len, "%u", va_arg(args, int32_t));
            result += buffer;
            break;
        case 'U':
            snprintf(buffer, buf_len, "%llu", va_arg(args, int64_t));
            result += buffer;
            break;
        case 'c':
            result += va_arg(args, uint32_t);
            break;
        case 'C':
            result += (uint32_t)va_arg(args, uint64_t);
            break;
        case 'f':
            snprintf(buffer, buf_len, "%g", va_arg(args, double));
            result += buffer;
            break;
        case 's':
            result += va_arg(args, const char *);
            break;
        case 'b':
            result += va_arg(args, bool) ? "true" : "false";
            break;
        case 'r':
            {
                std::complex<float> value = va_arg(args, std::complex<float>);
                snprintf(buffer, buf_len, "%g + %gi", value.real(), value.imag());
                result += buffer;
            }            
            break;
        case 'R':
            {
                std::complex<double> value = va_arg(args, std::complex<double>);
                snprintf(buffer, buf_len, "%g + %gi", value.real(), value.imag());
                result += buffer;
            }
            break;
        }
    }
    va_end(args);

    return(result);
}

/*
string format(const char *format, ...)
{

    va_start

    for (;count != 0; --count) {
        int i = ;
            std::cout << i << '\n';
        } else if (*fmt == 'c') {
            // note automatic conversion to integral type
            int c = va_arg(args, int);
            std::cout << static_cast<char>(c) << '\n';
        } else if (*fmt == 'f') {
            double d = va_arg(args, double);
            std::cout << d << '\n';
        }
        ++fmt;
    }

}
*/

} // namespace