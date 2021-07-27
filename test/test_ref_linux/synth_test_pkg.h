#pragma once

#include <sing.h>

namespace aa {
namespace bb {

typedef int32_t pkg_type;
static const int32_t pkg_ctc = 100;
typedef sing::array<int32_t, pkg_ctc> pkg_vectype;

int32_t pkg_fun(const pkg_vectype &p0);

}   // namespace
}   // namespace
