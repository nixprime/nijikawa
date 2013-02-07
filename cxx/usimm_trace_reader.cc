#include <cinttypes>
#include <cstdint>
#include <stdexcept>
#include <system_error>

#include "usimm_trace_reader.h"

namespace nijikawa {

UsimmTraceReader::UsimmTraceReader(std::string const& filename) {
  file_ = std::fopen(filename.c_str(), "r");
  if (!file_) {
    throw std::system_error(errno, std::system_category(), "Error opening "
        "trace file");
  }
}

TraceRecord UsimmTraceReader::next() {
  TraceRecord record;
  // Each line can have at most 52 characters:
  // - 32-bit decimal prec field => 10 characters
  // - Space => 1 character
  // - 'R' or 'W' => 1 character
  // - Space => 1 character
  // - "0x" => 2 characters
  // - 64-bit hexadecimal memory address field => 16 characters
  // - Space => 1 character
  // - "0x" => 2 characters
  // - 64-bit hexadecimal PC field => 16 characters
  // - "\r\n" => 2 characters
  // (53 bytes counting the terminating null)
  char buf[53];
  if (!std::fgets(buf, 53, file_)) {
    throw std::runtime_error("Error reading trace file: fgets returned null");
  }
  char req_type;
  Address pc;
  int fields = std::sscanf(buf, "%" SCNu64 " %c 0x%" SCNx64 " 0x%" SCNx64,
      &record.prec, &req_type, &record.addr, &pc);
  if (fields != 3 && fields != 4) {
    throw std::runtime_error("Error reading trace file: only read " +
        std::to_string(fields) + " fields");
  }
  switch (req_type) {
    case 'R':
      record.is_write = false;
      break;
    case 'W':
      record.is_write = true;
      break;
    default:
      throw std::runtime_error("Trace contains unknown request type " +
          std::string(&req_type, 1));
  }
  return record;
}

} // namespace nijikawa
