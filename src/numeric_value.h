#ifndef NUMERIC_VALUE_H
#define NUMERIC_VALUE_H

#include <stdint.h>
#include "lexer.h"

namespace SingNames {

class NumericValue {
public:
    enum OpError {
        OE_OK, OE_ILLEGAL_OP, OE_INTEGER_POWER_WRONG, OE_OVERFLOW, OE_INTEGER_OVERFLOW, OE_NAN_RESULT,
        OE_SHIFT_TOO_BIG, OE_DIV_BY_0, OE_NEGATIVE_SHIFT, OE_SIGNED_UNSIGNED_MISMATCH
    };

    NumericValue(NumericValue *other) : unsigned_(other->unsigned_), signed_(other->signed_), float_(other->float_), img_(other->img_) {}
    NumericValue() { Clear(); }
    void Clear(void);

    void InitFromFloatString(const char *str);
    void InitImgPartFromFloatString(const char *str);
    void InitFromUnsignedString(const char *str);
    bool InitFromStringsAndFlags(const char *real_part, const char *img_part, bool real_is_int, bool negate_real, bool negate_img);
    void InitFromInt32(int32_t value) { Clear(); signed_ = value; }

    OpError PerformOp(NumericValue *other, Token op);
    OpError PerformTypedOp(NumericValue *other, Token op, Token precision);
    OpError PerformConversion(Token op);
    OpError PerformTypedUnop(Token op, Token precision);

    // is the conversion possible without range or precision loss ?
    bool FitsUnsigned(int nbits)const;
    bool FitsSigned(int nbits)const;
    bool FitsFloat(bool is_double)const;
    bool FitsComplex(bool is_double)const;

    bool IsAPositiveInteger(void)const;
    bool IsAnIntegerExponent(void)const;
    bool IsComplex(void) const { return(img_ != 0); }
    bool IsAValidCharValue(void)const;
    uint64_t GetUnsignedIntegerValue(void) const { return(signed_ + unsigned_); }
    int64_t GetSignedIntegerValue(void) const { return((int64_t)(signed_ + unsigned_)); }
    double GetDouble(void) const { return(float_ + unsigned_ + signed_); }

private:
    uint64_t    unsigned_;
    int64_t     signed_;
    double      float_;
    double      img_;

    OpError PerformIntOp(NumericValue *other, Token op, int nbits, bool is_signed);
    OpError PerformFloatOp(NumericValue *other, Token op);
    OpError PerformDoubleOp(NumericValue *other, Token op);
    OpError PerformComplexOp(NumericValue *other, Token op);
    OpError PerformDoubleComplexOp(NumericValue *other, Token op);

    OpError ConvertToInt(int nbits, bool is_signed);
    bool NeedsToOperateOnFloats(NumericValue *other) { return(float_ != 0 || img_ != 0 || other->float_ != 0 || other->img_ != 0); }
    bool NeedsToOperateOnComplex(NumericValue *other) { return(img_ != 0 || other->img_ != 0); }
    void GetIntegerModAndSign(uint64_t *abs, bool *negative);
    bool SetIntegerModAndSign(uint64_t abs, bool negative);
    bool SetModAndSign(uint64_t abs, bool negative, int nbits, bool dst_is_signed);
    int MaxShift(int nbits, bool is_signed);
    OpError CheckFloatOverflow(void);
    void InitWithDouble(double value) { Clear(); float_ = value; }
};

}

#endif
