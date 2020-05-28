#include <stdlib.h>
#include <complex>
#include <assert.h>
#include <string.h>
#include <float.h>
#include "numeric_value.h"

namespace SingNames {

//                            ----XXXX----XXXX
static const uint64_t msb = 0x8000000000000000;

void NumericValue::Clear(void)
{
    unsigned_ = 0;
    signed_ = 0;
    float_ = 0;
    img_ = 0;
}

void NumericValue::GetIntegerModAndSign(uint64_t *abs, bool *negative)
{
    if (signed_ < 0) {
        *abs = -signed_;
        *negative = true;
    } else if (signed_ > 0) {
        *abs = signed_;
        *negative = false;
    } else {
        *abs = unsigned_;
        *negative = false;
    }
}

bool NumericValue::SetIntegerModAndSign(uint64_t abs, bool negative)
{
    if (negative) {
        if (abs > msb) {
            return(false);
        }
        signed_ = -(int64_t)abs;
        unsigned_ = 0;
    } else if (abs >= msb) {
        signed_ = 0;
        unsigned_ = abs;
    } else if (signed_ != 0) {
        signed_ = abs;
    } else {
        unsigned_ = abs;
    }
    return(true);
}

bool NumericValue::SetModAndSign(uint64_t abs, bool negative, int nbits, bool dst_is_signed)
{
    // nbits == 0 -> save as fit
    if (nbits == 0) {
        return(SetIntegerModAndSign(abs, negative));
    }
    if (dst_is_signed) {
        uint64_t max = (uint64_t)1 << (nbits - 1);
        if (negative) {
            if (abs > max) return(false);
            signed_ = -(int64_t)abs;
        } else {
            if (abs >= max) return(false);
            signed_ = (int64_t)abs;
        }
    } else {
        if (negative) return(false);
        if (nbits < 64) {
            uint64_t max = (uint64_t)1 << nbits;
            if (abs >= max) return(false);
        }
        unsigned_ = abs;
    }
    return(true);
}

void NumericValue::InitFromFloatString(const char *str)
{
    Clear();
    if (strchr(str, '_') != nullptr) {
        string copy = str;

        copy.erase_occurrencies_of('_');
        float_ = atof(copy.c_str());
    } else {
        float_ = atof(str);
    }
}

void NumericValue::InitImgPartFromFloatString(const char *str)
{
    Clear();
    if (strchr(str, '_') != nullptr) {
        string copy = str;

        copy.erase_occurrencies_of('_');
        img_ = atof(copy.c_str());
    } else {
        img_ = atof(str);
    }
}

void NumericValue::InitFromUnsignedString(const char *str)
{
    Clear();
    if (strchr(str, '_') != nullptr) {
        string copy = str;

        copy.erase_occurrencies_of('_');
        unsigned_ = strtoull(copy.c_str(), NULL, 0);
    } else {
        unsigned_ = strtoull(str, NULL, 0);
    }
    if (unsigned_ < msb) {
        signed_ = (int64_t) unsigned_;
        unsigned_ = 0;
    }
}

bool NumericValue::InitFromStringsAndFlags(const char *real_part, const char *img_part, bool real_is_int, bool negate_real, bool negate_img)
{
    string clean_real_part = real_part;
    string clean_img_part = img_part;

    clean_real_part.erase_occurrencies_of('_');
    clean_img_part.erase_occurrencies_of('_');

    Clear();
    if (real_is_int) {
        unsigned_ = strtoull(clean_real_part.c_str(), NULL, 0);
        if (negate_real) {
            if (unsigned_ > msb) {
                return(false);
            }
            signed_ = -(int64_t)unsigned_;
            unsigned_ = 0;
        }
    } else {
        float_ = atof(clean_real_part.c_str());
        if (negate_real) {
            float_ = -float_;
        }
    }
    img_ = atof(clean_img_part.c_str());
    if (negate_img) {
        img_ = -img_;
    }
    return(true);
}

// automatically selects a precision to try to preserve the value
NumericValue::OpError NumericValue::PerformOp(NumericValue *other, Token op)
{
    if (img_ != 0 || other->img_ != 0) {
        return(PerformDoubleComplexOp(other, op));
    } else if (float_ != 0 || other->float_ != 0) {
        return(PerformDoubleOp(other, op));
    } else {

        // can inputs and outputs be represented by the same integer type ?
        bool fits_signed = FitsSigned(64) && other->FitsSigned(64);
        bool fits_unsigned = FitsUnsigned(64) && other->FitsUnsigned(64);
        if (!fits_signed && !fits_unsigned) {
            return(OE_SIGNED_UNSIGNED_MISMATCH);
        }

        return(PerformIntOp(other, op, 0, fits_signed));
    }
}

//
// note: 
// if you call this function the following must be guaranteed:
//
// 1 - both 'this' and 'other' must contain values that are in range with 'precision' (the basic type).
// 2 - if 'precision' is an integer, the value must be stored in signed_ or unsigned_.
//
NumericValue::OpError NumericValue::PerformTypedOp(NumericValue *other, Token op, Token precision)
{
    switch (precision) {
    case TOKEN_INT8:
        assert(false);                              // shouldn't happen because of integer promotion
        return(PerformIntOp(other, op, 8, true));
    case TOKEN_INT16:
        assert(false);                              // shouldn't happen because of integer promotion
        return(PerformIntOp(other, op, 16, true));
    case TOKEN_INT32:
        return(PerformIntOp(other, op, 32, true));
    case TOKEN_INT64:
        return(PerformIntOp(other, op, 64, true));
    case TOKEN_UINT8:
        assert(false);                              // shouldn't happen because of integer promotion
        return(PerformIntOp(other, op, 8, false));
    case TOKEN_UINT16:
        assert(false);                              // shouldn't happen because of integer promotion
        return(PerformIntOp(other, op, 16, false));
    case TOKEN_UINT32:
        return(PerformIntOp(other, op, 32, false));
    case TOKEN_UINT64:
        return(PerformIntOp(other, op, 64, false));
    case TOKEN_FLOAT32:
        return(PerformFloatOp(other, op));
    case TOKEN_FLOAT64:
        return(PerformDoubleOp(other, op));
    case TOKEN_COMPLEX64:
        return(PerformComplexOp(other, op));
    case TOKEN_COMPLEX128:
        return(PerformDoubleComplexOp(other, op));
    default:
        break;
    }
    return(OE_ILLEGAL_OP);
}

NumericValue::OpError NumericValue::PerformConversion(Token op)
{
    switch (op) {
    case TOKEN_INT8:
        return(ConvertToInt(8, true));
    case TOKEN_INT16:
        return(ConvertToInt(16, true));
    case TOKEN_INT32:
        return(ConvertToInt(32, true));
    case TOKEN_INT64:
        return(ConvertToInt(64, true));
    case TOKEN_UINT8:
        return(ConvertToInt(8, false));
    case TOKEN_UINT16:
        return(ConvertToInt(16, false));
    case TOKEN_UINT32:
        return(ConvertToInt(32, false));
    case TOKEN_UINT64:
        return(ConvertToInt(64, false));
    case TOKEN_FLOAT32:
        InitWithDouble((float)GetDouble());
        break;
    case TOKEN_FLOAT64:
        InitWithDouble(GetDouble());
        break;
    case TOKEN_COMPLEX64:
        float_ = (float)GetDouble();
        img_ = (float)img_;
        signed_ = 0;
        unsigned_ = 0;
        break;
    case TOKEN_COMPLEX128:
        float_ = GetDouble();
        signed_ = 0;
        unsigned_ = 0;
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    return(OE_OK);
}

NumericValue::OpError NumericValue::PerformTypedUnop(Token op, Token precision)
{
    switch (op) {
    case TOKEN_MINUS:
        img_ = -img_;
        float_ = -float_;
        if (unsigned_ > msb || (uint64_t)signed_ == msb) {
            return(OE_INTEGER_OVERFLOW);    // too big to store it's negative value 
        } else {
            signed_ = -(signed_ + (int64_t)unsigned_);
            unsigned_ = 0;

            // note: 64 bits overflow already tested, 
            // smaller precisions not used because of integer promotion, unsigned unallowed.
            if (precision == TOKEN_INT32 && !FitsSigned(32)) {
                return(OE_INTEGER_OVERFLOW);
            }
        }
        break;
    case TOKEN_NOT:
        if (img_ != 0 || float_ != 0) {
            return(OE_ILLEGAL_OP);
        }
        switch (precision) {
        case TOKEN_INT8:
        case TOKEN_INT16:
        case TOKEN_INT32:
        case TOKEN_INT64:
            signed_ = ~signed_;
            break;
        case TOKEN_UINT8:
            unsigned_ = ~unsigned_ & 0xff;
            break;
        case TOKEN_UINT16:
            unsigned_ = ~unsigned_ & 0xffff;
            break;
        case TOKEN_UINT32:
            unsigned_ = ~unsigned_ & 0xffffffff;
            break;
        case TOKEN_UINT64:
            unsigned_ = ~unsigned_;
            break;
        }
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    return(OE_OK);
}

NumericValue::OpError NumericValue::PerformIntOp(NumericValue *other, Token op, int nbits, bool is_signed)
{
    uint64_t    op1mod, op2mod, result_mod;
    bool        op1neg, op2neg, result_neg;

    GetIntegerModAndSign(&op1mod, &op1neg);
    other->GetIntegerModAndSign(&op2mod, &op2neg);

    switch (op) {
    case TOKEN_POWER:
    {
        bool baseis0 = op1mod == 0;
        bool baseis1 = op1mod == 1 && !op1neg;
        bool baseisminus1 = op1mod == 1 && op1neg;
        bool expis0 = op2mod == 0;
        bool expis1 = op2mod == 1 && !op2neg;
        bool expisminus1 = op2mod == 1 && op2neg;

        // special cases
        if (baseis0 && (expis0 || op2neg)) {
            return(OE_INTEGER_POWER_WRONG); // 0**0 is undefined 0**negative is infinite
        }
        if (expisminus1 && op1mod == 1 || expis1 || baseis0 || baseis1 && !op2neg) {
            break;      // any**1, -1**-1, 1**-1, 0**positive, 1**positive : result is identical to base
        }
        if (expis0) {
            unsigned_ = 1;
            signed_ = 0;
            break;
        }

        // negative exponent causes a non integer result, exp > 63 causes overflow with any base
        if (op2neg || op2mod > 63) {
            return(OE_INTEGER_POWER_WRONG);
        }

        // compute result with successive multiplications
        result_mod = op1mod;
        result_neg = op1neg && (op2mod & 1) != 0;
        for (uint64_t ii = op2mod - 1; ii > 0; --ii) {
            uint64_t temp = result_mod * op1mod;
            if (temp / op1mod != result_mod) {
                return(OE_INTEGER_OVERFLOW);
            }
            result_mod = temp;
        }
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
    }
    break;
    case TOKEN_PLUS:
        if (op1neg == op2neg) {
            result_mod = op1mod + op2mod;
            if (op1mod > result_mod || op2mod > result_mod) {
                return(OE_INTEGER_OVERFLOW);
            }
            result_neg = op1neg;
        } else if (op1mod > op2mod) {
            result_mod = op1mod - op2mod;
            result_neg = op1neg;
        } else {
            result_mod = op2mod - op1mod;
            result_neg = op2neg;
        }
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    case TOKEN_MINUS:
        if (op1neg != op2neg) {
            result_mod = op1mod + op2mod;
            if (op1mod > result_mod || op2mod > result_mod) {
                return(OE_INTEGER_OVERFLOW);
            }
            result_neg = op1neg;
        } else if (op1mod > op2mod) {
            result_mod = op1mod - op2mod;
            result_neg = op1neg;
        } else {
            result_mod = op2mod - op1mod;
            result_neg = !op1neg;
        }
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    case TOKEN_MPY:
        result_mod = op1mod * op2mod;
        if (op1mod != 0 && result_mod / op1mod != op2mod) {
            return(OE_INTEGER_OVERFLOW);
        }
        result_neg = op1neg != op2neg;
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    case TOKEN_DIVIDE:
        if (op2mod == 0) {
            return(OE_DIV_BY_0);
        }
        // select signed or unsigned division (we can do both but c++ does one of the twos)
        if (op1neg || op2neg) {
            // signed
            if (op1mod > msb || !op1neg && op1mod == msb || op2mod > msb || !op2neg && op2mod == msb) {
                return(OE_INTEGER_OVERFLOW);
            }
        }
        result_mod = op1mod / op2mod;
        result_neg = op1neg != op2neg;
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    case TOKEN_MOD:
        result_mod = op1mod % op2mod;
        if (!SetModAndSign(result_mod, op1neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    case TOKEN_SHR:
        if (op2mod > 63) {
            return(OE_SHIFT_TOO_BIG);
        } else if (op2neg) {
            return(OE_NEGATIVE_SHIFT);
        }
        if (is_signed) {
            signed_ = (int64_t)(signed_ | unsigned_) >> op2mod;
        } else {
            unsigned_ = (signed_ | unsigned_) >> op2mod;
        }
        break;
    case TOKEN_SHL:
        if (op2mod > 63) {
            return(OE_SHIFT_TOO_BIG);
        } else if (op2neg) {
            return(OE_NEGATIVE_SHIFT);
        } else if (op2mod > MaxShift(nbits, is_signed)) {
            return(OE_SHIFT_TOO_BIG);
        }
        if (is_signed) {
            signed_ = (int64_t)(signed_ | unsigned_) << op2mod;
        } else {
            unsigned_ = (signed_ | unsigned_) << op2mod;
        }
        break;
    case TOKEN_AND:
        if (is_signed) {
            signed_ = (unsigned_ | signed_) & (other->signed_ | other->unsigned_);
            unsigned_ = 0;
        } else {
            unsigned_ = (unsigned_ | signed_) & (other->signed_ | other->unsigned_);
            signed_ = 0;
        }
        break;
    case TOKEN_OR:
        if (is_signed) {
            signed_ = (unsigned_ | signed_) | (other->signed_ | other->unsigned_);
            unsigned_ = 0;
        } else {
            unsigned_ = (unsigned_ | signed_) | (other->signed_ | other->unsigned_);
            signed_ = 0;
        }
        break;
    case TOKEN_XOR:
        if (is_signed) {
            signed_ = (unsigned_ | signed_) ^ (other->signed_ | other->unsigned_);
            unsigned_ = 0;
        } else {
            unsigned_ = (unsigned_ | signed_) ^ (other->signed_ | other->unsigned_);
            signed_ = 0;
        }
        break;
    case TOKEN_MIN:
        if (op1neg && op2neg && op2mod > op1mod || !op1neg && op2neg || !op1neg && !op2neg && op2mod < op1mod) {
            result_mod = op2mod;
            result_neg = op2neg;
        } else {
            result_mod = op1mod;
            result_neg = op1neg;
        }
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    case TOKEN_MAX: 
        if (op1neg && op2neg && op2mod > op1mod || !op1neg && op2neg || !op1neg && !op2neg && op2mod < op1mod) {
            result_mod = op1mod;
            result_neg = op1neg;
        } else {
            result_mod = op2mod;
            result_neg = op2neg;
        }
        if (!SetModAndSign(result_mod, result_neg, nbits, is_signed)) {
            return(OE_INTEGER_OVERFLOW);
        }
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    return(OE_OK);
}

NumericValue::OpError NumericValue::PerformFloatOp(NumericValue *other, Token op)
{
    float op1 = (float)GetDouble();
    float op2 = (float)other->GetDouble();
    switch (op) {
    case TOKEN_POWER:
        float_ = pow(op1, op2);
        break;
    case TOKEN_PLUS:
        float_ = op1 + op2;
        break;
    case TOKEN_MINUS:
        float_ = op1 - op2;
        break;
    case TOKEN_MPY:
        float_ = op1 * op2;
        break;
    case TOKEN_DIVIDE:
        if (op2 == 0) {
            return(OE_DIV_BY_0);
        }
        float_ = op1 / op2;
        break;
    case TOKEN_MIN:
        float_ = std::min(op1, op2);
        break;
    case TOKEN_MAX:
        float_ = std::max(op1, op2);
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    unsigned_ = 0;
    signed_ = 0;
    return(CheckFloatOverflow());
}

NumericValue::OpError NumericValue::PerformDoubleOp(NumericValue *other, Token op)
{
    double op1 = GetDouble();
    double op2 = other->GetDouble();
    switch (op) {
    case TOKEN_POWER:
        float_ = pow(op1, op2);
        break;
    case TOKEN_PLUS:
        float_ = op1 + op2;
        break;
    case TOKEN_MINUS:
        float_ = op1 - op2;
        break;
    case TOKEN_MPY:
        float_ = op1 * op2;
        break;
    case TOKEN_DIVIDE:
        if (op2 == 0) {
            return(OE_DIV_BY_0);
        }
        float_ = op1 / op2;
        break;
    case TOKEN_MIN:
        float_ = std::min(op1, op2);
        break;
    case TOKEN_MAX:
        float_ = std::max(op1, op2);
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    unsigned_ = 0;
    signed_ = 0;
    return(CheckFloatOverflow());
}

NumericValue::OpError NumericValue::PerformComplexOp(NumericValue *other, Token op)
{
    std::complex<float> x, y;
    x.real((float)GetDouble());
    x.imag((float)img_);
    y.real((float)other->GetDouble());
    y.imag((float)other->img_);
    switch (op) {
    case TOKEN_POWER:
        x = pow(x, y);
        break;
    case TOKEN_PLUS:
        x += y;
        break;
    case TOKEN_MINUS:
        x -= y;
        break;
    case TOKEN_MPY:
        x *= y;
        break;
    case TOKEN_DIVIDE:
        if (y.real() == 0 && y.imag() == 0) {
            return(OE_DIV_BY_0);
        }
        x /= y;
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    unsigned_ = 0;
    signed_ = 0;
    float_ = x.real();
    img_ = x.imag();
    return(CheckFloatOverflow());
}

NumericValue::OpError NumericValue::PerformDoubleComplexOp(NumericValue *other, Token op)
{
    std::complex<double> x, y;
    x.real(GetDouble());
    x.imag(img_);
    y.real(other->GetDouble());
    y.imag(other->img_);
    switch (op) {
    case TOKEN_POWER:
        x = pow(x, y);
        break;
    case TOKEN_PLUS:
        x += y;
        break;
    case TOKEN_MINUS:
        x -= y;
        break;
    case TOKEN_MPY:
        x *= y;
        break;
    case TOKEN_DIVIDE:
        if (y.real() == 0 && y.imag() == 0) {
            return(OE_DIV_BY_0);
        }
        x /= y;
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    unsigned_ = 0;
    signed_ = 0;
    float_ = x.real();
    img_ = x.imag();
    return(CheckFloatOverflow());
}

NumericValue::OpError NumericValue::ConvertToInt(int nbits, bool is_signed)
{
    img_ = 0;
    if (float_ == 0) {
        if (is_signed) {
            signed_ = unsigned_ | signed_;
            unsigned_ = 0;
            switch (nbits) {
            case 8:
                signed_ = (int64_t)(int8_t)signed_;
                break;
            case 16:
                signed_ = (int64_t)(int16_t)signed_;
                break;
            case 32:
                signed_ = (int64_t)(int32_t)signed_;
                break;
            default:
                break;
            }
        } else {
            unsigned_ = unsigned_ + signed_;
            signed_ = 0;
            if (nbits < 64) {
                unsigned_ &= (1ll << nbits) - 1;
            }
        }
    } else {
        bool tested = false;

        signed_ = unsigned_ = 0;

        // if it is so big it can't have a fractional part,
        // you can call FitsSigned() safely (it wont fail because of a fractional part)
        // on the other end if it is smaller, it is guaranteed to fit an int64 and can be tested later
        if (float_ > 1e17 || float_ < -1e17) {
            if (is_signed && !FitsSigned(nbits) || !is_signed && !FitsUnsigned(nbits)) {
                return(OE_OVERFLOW);
            }
            tested = true;
        }
        if (is_signed) {
            signed_ = (int64_t)float_;
        } else {
            unsigned_ = (uint64_t)float_;
        }
        float_ = 0;
        if (!tested) {
            if (is_signed && !FitsSigned(nbits) || !is_signed && !FitsUnsigned(nbits)) {
                return(OE_OVERFLOW);
            }
        }
    }
    return(OE_OK);
}

bool NumericValue::FitsUnsigned(int nbits) const
{
    uint64_t value;
    uint64_t cast_value;

    if (img_ != 0 || signed_ < 0) return(false);
    if (float_ != 0) {
        value = (uint64_t)float_;
        if (value != float_) return(false);
    } else {
        value = signed_ | unsigned_;
    }
    if (nbits == 64) return(true);
    cast_value = value & ((uint64_t)1 << nbits) - 1;
    return(cast_value == value);
}

bool NumericValue::FitsSigned(int nbits) const
{
    int64_t value;
    int64_t max;

    if (img_ != 0 || unsigned_ >= msb) return(false);
    if (float_ != 0) {
        value = (int64_t)float_;
        if (value != float_) return(false);
    } else {
        value = signed_ | unsigned_;
    }
    if (nbits == 64) return(unsigned_ < msb);
    max = (uint64_t)1 << (nbits - 1);
    return(value < max && value >= -max);
}

bool NumericValue::FitsFloat(bool is_double) const
{
    if (img_ != 0) return(false);
    return(FitsComplex(is_double));
}

bool NumericValue::FitsComplex(bool is_double) const
{
    if (is_double) {
        if (unsigned_ > 0x20000000000000 || signed_ > 0x20000000000000 || signed_ < -0x20000000000000) {
            return(false);
        }
    } else {
        if (unsigned_ >= 0x1000000 || signed_ > 0x1000000 || signed_ < -0x1000000) {
            return(false);
        }
        if (float_ > FLT_MAX || float_ < -FLT_MAX || img_ > FLT_MAX || img_ < -FLT_MAX) {
            return(false);
        }
    }
    return(true);
}

bool NumericValue::IsAPositiveInteger(void) const
{
    return (img_ == 0 && float_ == 0 && signed_ >= 0);
}

bool NumericValue::IsAnIntegerExponent(void) const
{
    return (IsAPositiveInteger() && signed_ + unsigned_ <= 63);
}

bool NumericValue::IsAValidCharValue(void) const
{
    return (IsAPositiveInteger() && signed_ + unsigned_ <= 0x3fffff);
}

int NumericValue::MaxShift(int nbits, bool is_signed)
{
    uint64_t acc;
    uint64_t mask = (uint64_t)1 << (nbits - 1);
    int leading = 0;

    if (is_signed) {
        if (signed_ < 0) {
            acc = ~(uint64_t)signed_;
        } else {
            acc = (uint64_t)signed_;
        }
    } else {
        acc = unsigned_;
    }

    // leading 0s
    while ((acc & mask) == 0 && mask != 0) {
        ++leading;
        mask >>= 1;
    }

    if (is_signed) {
        if (leading == 0) return(nbits - 1);    // something went wrong (signed_ was in overflow ?)
        return(leading - 1);
    } 
    return(leading);
}

NumericValue::OpError NumericValue::CheckFloatOverflow(void)
{
    if (float_ != 0 && float_ * 2 == float_ || img_ != 0 && img_ * 2 == img_) {
        return(OE_OVERFLOW);
    }
    if (float_ != float_ || img_ != img_) {
        return(OE_NAN_RESULT);
    }
    return(OE_OK);
}

}   // namespace
