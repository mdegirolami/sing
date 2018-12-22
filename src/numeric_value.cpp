#include <stdlib.h>
#include <complex>
#include "numeric_value.h"

namespace SingNames {

static const uint64_t msb = 1 << 63;

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
        signed_ = -abs;
        unsigned_ = 0;
    } else if (abs >= msb) {
        signed_ = 0;
        unsigned_ = abs;
    } else if (signed_ != 0) {
        signed_ = negative ? -abs : abs;
    } else {
        unsigned_ = abs;
    }
    return(true);
}

void NumericValue::InitFromFloatString(const char *str)
{
    Clear();
    float_ = atof(str);
}

void NumericValue::InitImgPartFromFloatString(const char *str)
{
    Clear();
    img_ = atof(str);
}

void NumericValue::InitFromUnsignedString(const char *str)
{
    Clear();
    unsigned_ = strtoull(str, NULL, 0);
}

NumericValue::OpError NumericValue::PerformOp(NumericValue *other, Token op)
{
    if (img_ != 0 || other->img_ != 0) {
        std::complex<double> x, y;
        x.real = GetDouble();
        x.imag = img_;
        y.real = other->GetDouble();
        y.imag = other->img_;
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
            x /= y;
            break;
        default:
            return(OE_ILLEGAL_OP);
        }
        unsigned_ = 0;
        signed_ = 0;
        float_ = x.real;
        img_ = x.imag;
    } else if (float_ != 0 || other->float_ != 0) {
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
            float_ = op1 / op2;
            break;
        default:
            return(OE_ILLEGAL_OP);
        }
        unsigned_ = 0;
        signed_ = 0;
    } else {
        uint64_t op1mod, op2mod, result_mod;
        bool     op1neg, op2neg, result_neg;

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
                    return(OE_INTEGER_POWER_WRONG); // 0^0 is undefined 0^negative is infinite
                }  
                if (expisminus1 && op1mod == 1 || expis1 || baseis0 || baseis1 && !op2neg) {
                    break;      // any^1, -1^-1, 1^-1, 0^positive, 1^positive : result is identical to base
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
                for (int ii = op2mod - 1; ii > 0; --ii) {
                    uint64_t temp = result_mod * op1mod;
                    if (temp / op1mod != result_mod) {
                        return(OE_INTEGER_OVERFLOW);
                    }
                    result_mod = temp;
                }
                if (!SetIntegerModAndSign(result_mod, result_neg)) {
                    return(OE_INTEGER_OVERFLOW);
                }
            }
            break;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            {
                bool isplus = op == TOKEN_PLUS;
                bool different_signs = op1neg != op2neg;

                if (isplus != different_signs) {
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
                if (!SetIntegerModAndSign(result_mod, result_neg)) {
                    return(OE_INTEGER_OVERFLOW);
                }
            }
            break;
        case TOKEN_MPY:
            result_mod = op1mod * op2mod;
            if (result_mod / op1mod != op2mod) {
                return(OE_INTEGER_OVERFLOW);
            }
            result_neg = op1neg != op2neg;
            if (!SetIntegerModAndSign(result_mod, result_neg)) {
                return(OE_INTEGER_OVERFLOW);
            }
            break;
        case TOKEN_DIVIDE:
            // select signed or unsigned division (we can do both but c++ does one of the twos)
            if (op1neg || op2neg) {
                // signed
                if (op1mod > msb || !op1neg && op1mod == msb || op2mod > msb || !op2neg && op2mod == msb) {
                    return(OE_INTEGER_OVERFLOW);
                }
            }
            result_mod = op1mod / op2mod;
            result_neg = op1neg != op2neg;
            if (!SetIntegerModAndSign(result_mod, result_neg)) {
                return(OE_INTEGER_OVERFLOW);
            }
            break;
        case TOKEN_MOD:
            result_mod = op1mod % op2mod;
            if (!SetIntegerModAndSign(result_mod, op1neg)) {
                return(OE_INTEGER_OVERFLOW);
            }
            break;
        case TOKEN_SHR:
            {
                if (op2mod > 64) {
                    return(OE_SHIFT_TOO_BIG);
                }
                int shift = op2neg ? -op2mod : op2mod;
                signed_ >>= shift;
                unsigned_ >>= shift;
            }
            break;
        case TOKEN_SHL:
            {
                if (op2mod > 64) {
                    return(OE_SHIFT_TOO_BIG);
                }
                int shift = op2neg ? -op2mod : op2mod;
                signed_ <<= shift;
                unsigned_ <<= shift;
            }
            break;
        case TOKEN_AND:
            unsigned_ = (unsigned_ | signed_) & (other->signed_ | other->unsigned_);
            signed_ = 0;
            break;
        case TOKEN_OR:
            unsigned_ = (unsigned_ | signed_) | (other->signed_ | other->unsigned_);
            signed_ = 0;
            break;
        case TOKEN_XOR:
            unsigned_ = (unsigned_ | signed_) ^ (other->signed_ | other->unsigned_);
            signed_ = 0;
            break;
        default:
            return(OE_ILLEGAL_OP);
        }
        return(OE_OK);
    }
    if (float_ * 2 == float_ || img_ * 2 == img_) {
        return(OE_OVERFLOW);
    }
    if (float_ != float_ || img_ != img_) {
        return(OE_NAN_RESULT);
    }
    return(OE_OK);
}

NumericValue::OpError NumericValue::PerformUnop(Token op)
{
    switch (op) {
    case TOKEN_MINUS:
        img_ = -img_;
        float_ = -float_;
        if (unsigned_ > msb) {
            return(OE_INTEGER_OVERFLOW);
        } else if (signed_ >= 0) {
            signed_ = -(signed_ + unsigned_);
            unsigned_ = 0;
        } else {
            unsigned_ = signed_ + unsigned_;
            signed_ = 0;
        }
        break;
    case TOKEN_NOT:
        if (img_ != 0 || float_ != 0) {
            return(OE_ILLEGAL_OP);
        }
        if (unsigned_ != 0) {
            unsigned_ = ~unsigned_;
        } else {
            signed_ = ~signed_;
        }
        break;
    default:
        return(OE_ILLEGAL_OP);
    }
    return(OE_OK);
}

bool NumericValue::FitsUnsigned(int nbits)
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
    cast_value = value & ((uint64_t)1 << nbits) - 1;
    return(cast_value == value);
}

bool NumericValue::FitsSigned(int nbits)
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
    if (nbits == 64) return(true);
    max = (uint64_t)1 << (nbits - 1);
    return(value < max && value >= -max);
}

bool NumericValue::FitsFloat(bool is_double)
{
    if (img_ != 0) return(false);
    return(is_double || float_ <= FLT_MAX && float_ >= FLT_MIN);
}

bool NumericValue::FitsComplex64(void)
{
    return(float_ <= FLT_MAX && float_ >= FLT_MIN && img_ <= FLT_MAX && img_ >= FLT_MIN);
}

bool NumericValue::IsAPositiveInteger(void)
{
    return (img_ == 0 && float_ == 0 && signed_ >= 0);
}

bool NumericValue::IsAnIntegerExponent(void)
{
    return (IsAPositiveInteger() && signed_ + unsigned_ <= 63);
}

bool NumericValue::IsAValidCharValue(void)
{
    return (IsAPositiveInteger() && signed_ + unsigned_ <= 0x3fffff);
}

}   // namespace
