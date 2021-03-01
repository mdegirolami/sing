#pragma once

#include <sing.h>

namespace sing {

static const int32_t i8_min = -0x80;
static const int32_t i8_max = +0x7f;
static const int32_t i16_min = -0x8000;
static const int32_t i16_max = +0x7fff;
static const int32_t i32_min = (int32_t)-0x80000000LL;
static const int32_t i32_max = +0x7fffffff;
static const int64_t i64_min = -0x8000000000000000LL;
static const int64_t i64_max = 0x7fffffffffffffffLL;

static const int32_t u8_max = +0xff;
static const int32_t u16_max = +0xffff;
static const uint32_t u32_max = 0xffffffffU;
static const uint64_t u64_max = 0xffffffffffffffffLLU;

extern const float f32_min;
extern const float f32_eps;
extern const float f32_max;
extern const double f64_min;
extern const double f64_eps;
extern const double f64_max;
extern const float pi32;
extern const double pi64;

}   // namespace
