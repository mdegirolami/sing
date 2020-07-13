#pragma once
#include <cstdint>

namespace sing {

static const int UpperLower = -0x80000000;  // upper-lower alternate !

struct CaseRange  {
	uint32_t low;
	uint32_t high;
	int32_t  deltas[3];
};

const CaseRange *get_record(int32_t cp);

} // namespace