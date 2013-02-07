#ifndef NIJIKAWA_TRACE_READER_H_
#define NIJIKAWA_TRACE_READER_H_

#include "trace_record.h"

namespace nijikawa {

class TraceReader {
  public:
    virtual TraceRecord next() = 0;
};

} // namespace nijikawa

#endif /* NIJIKAWA_TRACE_READER_H_ */
