use std::hashmap::HashMap;
use extra::priority_queue::*;

use mem_request::*;
use simulator::*;
use trace::*;
use util::cons_vec;

static CYCLE_INFINITY: i64 = 9223372036854775807i64;

pub struct Core<'self> {
    priv sim: &'self Simulator,
    priv trace_reader: &'self mut TraceReader,
    priv mem: &'self mut MemRequestReceiver,
    superscalar_width: uint,
    rob_size: uint,
    priv rob_head: uint,
    priv rob_tail: uint,
    priv rob_insns: uint,
    priv rob: ~[i64],
    priv cur_mem: TraceRecord,
    priv mshrs: HashMap<u64, Mshr>,
    priv waiting_responses: PriorityQueue<QueuedRequest>,
    insns_retired: u64
}

struct Mshr {
    addr: u64,
    issued: bool,
    rob_indices: ~[uint]
}

struct QueuedRequest {
    cycle: i64,
    response: ~MemResponse
}

impl<'self> Core<'self> {
    pub fn new(sim: &'self Simulator, trace_reader: &'self mut TraceReader,
               mem: &'self mut MemRequestReceiver, superscalar_width: uint,
               rob_size: uint) -> Core<'self> {
        let cur_mem = trace_reader.next_trace_record();
        Core { sim: sim, trace_reader: trace_reader, mem: mem,
               superscalar_width: superscalar_width, rob_size: rob_size,
               rob_head: 0, rob_tail: 0, rob_insns: 0,
               rob: cons_vec(rob_size, || { 0 }), cur_mem: cur_mem,
               mshrs: HashMap::new(), waiting_responses: PriorityQueue::new(),
               insns_retired: 0 }
    }

    pub fn tick(@mut self) {
        self.tick_retire();
        self.tick_mem();
        self.tick_issue();
    }

    fn tick_issue(@mut self) {
        let now = self.sim.now();
        let mut remaining = self.superscalar_width;
        while remaining > 0 && self.rob_insns < self.rob_size {
            if self.cur_mem.prec > 0 {
                self.rob[self.rob_tail] = now;
                self.cur_mem.prec -= 1;
            } else {
                if self.cur_mem.is_write {
                    self.issue_write(self.cur_mem.addr);
                    self.rob[self.rob_tail] = now;
                } else {
                    let mut mshr = self.get_mshr(self.cur_mem.addr);
                    mshr.rob_indices.push(self.rob_tail);
                    self.rob[self.rob_tail] = CYCLE_INFINITY;
                    self.issue_mshr(mshr);
                }
                self.cur_mem = self.trace_reader.next_trace_record();
            }
            remaining -= 1;
            self.rob_insns += 1;
            self.rob_tail += 1;
            if self.rob_tail >= self.rob_size {
                self.rob_tail = 0;
            }
        }
    }

    fn tick_mem(&mut self) {
        while !self.waiting_responses.is_empty() {
            let mut response = self.waiting_responses.top();
            if response.cycle > self.sim.now() {
                break;
            }
            self.deliver_mem_response(response.response);
            self.waiting_responses.pop();
        }
    }

    fn tick_retire(&mut self) {
        let now = self.sim.now();
        let mut remaining = self.superscalar_width;
        while remaining > 0 && self.rob_insns > 0 {
            if self.rob[self.rob_head] > now {
                break;
            }
            remaining -= 1;
            self.rob_insns -= 1;
            self.rob_head += 1;
            if self.rob_head >= self.rob_size {
                self.rob_head = 0;
            }
            self.insns_retired += 1;
        }
    }

    fn deliver_mem_response(&mut self, response: ~MemResponse) {
        for &i in self.mshrs.pop(&response.addr).unwrap().rob_indices.iter() {
            self.rob[i] = self.sim.now();
        }
    }

    fn get_mshr(&'self mut self, addr: u64) -> &'self mut Mshr {
        self.mshrs.find_or_insert_with(addr, |&addr| { Mshr::new(addr) })
    }

    fn issue_mshr(@mut self, mshr: &mut Mshr) {
        if !mshr.issued {
            self.mem.receive_request(~MemRequest {
                    addr: mshr.addr, req_type: Read,
                    response_receiver: Some(self as @mut MemResponseReceiver)
            });
            mshr.issued = true;
        }
    }

    fn issue_write(&mut self, addr: u64) {
        self.mem.receive_request(~MemRequest { addr: addr, req_type: Write,
                                               response_receiver: None });
    }
}

impl<'self> MemResponseReceiver for Core<'self> {
    fn receive_response(&mut self, cycle: i64, response: ~MemResponse) {
        self.waiting_responses.push(QueuedRequest { cycle: cycle,
                                                    response: response });
    }
}

impl Mshr {
    fn new(addr: u64) -> Mshr {
        Mshr { addr: addr, issued: false, rob_indices: ~[] }
    }
}

impl Ord for QueuedRequest {
    // Inverted because PriorityQueue is a max-prioq
    fn lt(&self, other: &QueuedRequest) -> bool { self.cycle > other.cycle }
}
