#ifndef NIJIKAWA_TRACE_RECORD_H_
#define NIJIKAWA_TRACE_RECORD_H_

#include <cstdint>

#include "address.h"

namespace nijikawa {

struct TraceRecord {
  /** Memory address. */
  Address addr;
  /** Number of preceding non-memory instructions. */
  std::size_t prec;
  /** True if the request is a write and false if the request is a read. */
  bool is_write;
};

} // namespace nijikawa

#endif /* NIJIKAWA_TRACE_RECORD_H_ */
