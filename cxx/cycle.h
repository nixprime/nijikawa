#ifndef NIJIKAWA_CYCLE_H_
#define NIJIKAWA_CYCLE_H_

#include <cstdint>
#include <limits>

namespace nijikawa {

typedef std::int64_t Cycle;
constexpr Cycle kMaxCycle = std::numeric_limits<Cycle>::max();

} // namespace nijikawa

#endif /* NIJIKAWA_CYCLE_H_ */
