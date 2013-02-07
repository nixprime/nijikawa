package com.nixprime.nijikawa.component

trait TraceReader {
  def next(): Option[TraceRecord]
}
