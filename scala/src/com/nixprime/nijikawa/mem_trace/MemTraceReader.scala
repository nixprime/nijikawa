package com.nixprime.nijikawa.mem_trace

trait MemTraceReader {
  def next(): Option[MemTraceRecord]
}
