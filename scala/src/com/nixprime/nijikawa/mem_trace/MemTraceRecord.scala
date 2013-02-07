package com.nixprime.nijikawa.mem_trace

import com.nixprime.nijikawa.common.Address

class MemTraceRecord(val addr: Address, val prec: Long, val isWrite: Boolean) {
}
