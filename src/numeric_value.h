#ifndef NUMERIC_VALUE_H
#define NUMERIC_VALUE_H

#include <stdint.h>
#include "lexer.h"

namespace SingNames {

class NumericValue {
public:
    enum OpError {
        OE_OK, OE_ILLEGAL_OP, OE_INTEGER_POWER_WRONG, OE_OVERFLOW, OE_INTEGER_OVERFLOW, OE_NAN_RESULT,
        OE_SHIFT_TOO_BIG
    };

    NumericValue(NumericValue *other) : unsigned_(other->unsigned_), signed_(other->signed_), float_(other->float_), img_(other->img_) {}
    NumericValue() { Clear(); }
    void Clear(void);

    void InitFromFloatString(const char *str);
    void InitImgPartFromFloatString(const char *str);
    void InitFromUnsignedString(const char *str);

    OpError PerformOp(NumericValue *other, Token op);
    OpError PerformUnop(Token op);

    bool FitsUnsigned(int nbits);
    bool FitsSigned(int nbits);
    bool FitsFloat(bool is_double);
    bool FitsComplex64(void);
    bool IsAPositiveInteger(void);
    bool IsAnIntegerExponent(void);
    bool IsComplex(void) { return(img_ != 0); }
    bool IsAValidCharValue(void);
    uint64_t GetPositiveIntegerValue(void) { return(signed_ + unsigned_); }


private:
    uint64_t    unsigned_;
    int64_t     signed_;
    double      float_;
    double      img_;

    bool NeedsToOperateOnFloats(NumericValue *other) { return(float_ != 0 || img_ != 0 || other->float_ != 0 || other->img_ != 0); }
    bool NeedsToOperateOnComplex(NumericValue *other) { return(img_ != 0 || other->img_ != 0); }
    double GetDouble(void) { return(float_ + unsigned_ + signed_); }
    void GetIntegerModAndSign(uint64_t *abs, bool *negative);
    bool SetIntegerModAndSign(uint64_t abs, bool negative);
};

}

#endif
