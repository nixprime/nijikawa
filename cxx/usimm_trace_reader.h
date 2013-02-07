#ifndef NIJIKAWA_USIMM_TRACE_READER_H_
#define NIJIKAWA_USIMM_TRACE_READER_H_

#include <cstdio>
#include <string>

#include "trace_reader.h"

namespace nijikawa {

class UsimmTraceReader : public TraceReader {
  public:
    explicit UsimmTraceReader(std::string const& filename);

    virtual TraceRecord next() final override;

  private:
    FILE* file_;
};

} // namespace nijikawa

#endif /* NIJIKAWA_USIMM_TRACE_READER_H_ */
