#include <cstdarg>
#include <sing.h>

//#define string std::string

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
            va_arg(args, int);
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
            result += va_arg(args, int) ? "true" : "false";
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

std::string sfmt(const char *format, ...)
{
    va_list     args;
    std::string retval;
    const char  *scan;
    size_t      total_bytes = 0;
    std::string result;
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
            va_arg(args, int);
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
            snprintf(buffer, buf_len, "%f", va_arg(args, double));
            result += buffer;
            break;
        case 's':
            result += va_arg(args, const char *);
            break;
        case 'b':
            result += va_arg(args, int) ? "true" : "false";
            break;
        case 'r':
            {
                std::complex<float> value = va_arg(args, std::complex<float>);
                snprintf(buffer, buf_len, "%f + %fi", value.real(), value.imag());
                result += buffer;
            }            
            break;
        case 'R':
            {
                std::complex<double> value = va_arg(args, std::complex<double>);
                snprintf(buffer, buf_len, "%f + %fi", value.real(), value.imag());
                result += buffer;
            }
            break;
        }
    }
    va_end(args);

    return(result);
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

// note: typically contex is a vector or a class with vector[s]
void quick_sort_indices(int *vv, int count, int(*comp)(int, int, void *), void *context)
{
    int tmp, lower, upper, pivot;

    // trivial cases
    if (count < 2) return;
    if (count == 2) {
        if (comp(vv[0], vv[1], context) > 0) {
            tmp = vv[1];
            vv[1] = vv[0];
            vv[0] = tmp;
        }
        return;
    }

    // sort around the pivot
    lower = 0;
    upper = count - 1;
    pivot = count >> 1;
    while (true) {

        // find an item preceeding the pivot that should stay after the pivot.
        while (lower < pivot && comp(vv[lower], vv[pivot], context) <= 0) {
            ++lower;
        }

        // find an item succeeding the pivot that should stay before the pivot.
        while (upper > pivot && comp(vv[upper], vv[pivot], context) >= 0) {
            --upper;
        }

        // swap them
        if (lower < pivot) {
            if (upper > pivot) {
                tmp = vv[lower];
                vv[lower] = vv[upper];
                vv[upper] = tmp;
                ++lower;
                --upper;
            } else {

                // lower is out of place but not upper.
                // move the pivot down one position to make room for lower
                tmp = vv[pivot];
                vv[pivot] = vv[lower];
                vv[lower] = vv[pivot - 1];
                vv[pivot - 1] = tmp;
                --pivot;
                upper = pivot;
            }
        } else {
            if (upper > pivot) {

                // upper is out of place but not lower.
                tmp = vv[pivot];
                vv[pivot] = vv[upper];
                vv[upper] = vv[pivot + 1];
                vv[pivot + 1] = tmp;
                ++pivot;
                lower = pivot;
            } else {
                break;
            }
        }
    }

    // recur
    if (pivot > 1) {
        quick_sort_indices(vv, pivot, comp, context);
    }
    tmp = count - pivot - 1;
    if (tmp > 1) {
        quick_sort_indices(vv + (pivot + 1), tmp, comp, context);
    }
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