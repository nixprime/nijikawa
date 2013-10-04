use std::u64;

pub struct TraceRecord {
    addr: u64,
    prec: u64,
    is_write: bool,
}

pub trait TraceReader {
    fn next_trace_record(&self) -> TraceRecord;
}
