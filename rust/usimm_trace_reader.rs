use std::from_str::*;
use io = std::io;
use std::path::PosixPath;

use trace::*;

pub struct UsimmTraceReader {
    priv reader: ~io::ReaderUtil,
}

impl UsimmTraceReader {
    pub fn new(filename: &str) -> UsimmTraceReader {
        UsimmTraceReader {
            reader: (~io::file_reader(&PosixPath(filename)).unwrap())
                    as ~ReaderUtil
        }
    }
}

impl TraceReader for UsimmTraceReader {
    fn next_trace_record(&self) -> TraceRecord {
        let line: ~str = self.reader.read_line();
        let words: ~[&str] = line.split_iter(' ').collect();
        assert!(words.len() == 3 || words.len() == 4);
        TraceRecord {
            addr: from_str(words[2]).unwrap(),
            prec: from_str(words[0]).unwrap(),
            is_write: match words[1] {
                "R" => false,
                "W" => true,
                _ => fail2!("unknown request type {:s}", words[1])
            }
        }
    }
}
