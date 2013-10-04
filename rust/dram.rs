use std::i64;

use mem_request::*;
use simulator::*;
use util::cons_vec;

static DRAM_OFFSET_BITS: uint = 6;
static DRAM_ROW_SIZE_BITS: uint = 13;

static NO_OPEN_ROW: u64 = !0;

pub struct Dram<'self> {
    priv sim: &'self Simulator,
    priv channel_bits: uint,
    priv bank_bits: uint,
    priv bank_lsb: uint,
    priv row_lsb: uint,
    clock_divider: i64,
    t_ccd: i64,
    t_cl: i64,
    t_rcd: i64,
    t_rp: i64,
    t_ras: i64,
    priv channels: ~[Channel],
}

struct Channel {
    banks: ~[Bank],
    waiting_reqs: ~[~Request],
    next_request: i64,
}

struct Bank {
    open_row: u64,
    next_request: i64,
    next_conflict: i64,
}

struct Request {
    channel: u64,
    bank: u64,
    row: u64,
    mem_req: ~MemRequest,
}

enum RequestConflictState {
    Hit,
    Miss,
    Conflict,
}

impl<'self> Dram<'self> {
    pub fn new(sim: &'self Simulator, channel_bits: uint, bank_bits: uint) ->
            Dram<'self> {
        let bank_lsb = DRAM_ROW_SIZE_BITS + channel_bits;
        let num_channels = 1u << channel_bits;
        let num_banks_per_channel = 1u << bank_bits;
        Dram {
            sim: sim,
            channel_bits: channel_bits,
            bank_bits: bank_bits,
            bank_lsb: bank_lsb,
            row_lsb: bank_lsb + bank_bits,
            clock_divider: 4,
            t_ccd: 4,
            t_cl: 11,
            t_rcd: 11,
            t_rp: 11,
            t_ras: 28,
            channels: cons_vec(num_channels,
                               || { Channel::new(num_banks_per_channel) }),
        }
    }

    pub fn tick(&mut self) {
        if self.sim.now() % self.clock_divider == 0 {
            for chan in self.channels.mut_iter() {
                if chan.next_request <= self.sim.now() {
                    match self.best_request(chan) {
                        Some(req) => { self.issue_request(req); }
                        None => {}
                    }
                }
            }
        }
    }

    fn best_request(&self, chan: &mut Channel) -> Option<~Request> {
        let now = self.sim.now();
        let mut best: Option<uint> = None;
        for i in range(0, chan.waiting_reqs.len()) {
            let req = &*chan.waiting_reqs[i];
            let bank = &chan.banks[req.bank];
            if bank.next_request > now {
                loop;
            }
            match self.request_conflict_state(req) {
                Hit => { best = Some(i); break; }
                Conflict if bank.next_conflict > now => { loop; }
                _ => match best {
                    Some(j) => {}
                    None => { best = Some(i); }
                }
            }
        }
        match best {
            Some(i) => Some(chan.waiting_reqs.remove(i)),
            None => None
        }
    }

    fn issue_request(&mut self, req: ~Request) {
        let chan = &self.channels[req.channel];
        let bank = chan.banks[req.bank];
        let req_state = self.request_conflict_state(req);
        let mut req_delay: i64 = 0;
        chan.next_request = self.after(self.t_ccd);
        match req_state { Conflict => { req_delay += self.t_rp; }, _ => {} }
        match req_state {
            Hit => {},
            _ => {
                bank.next_conflict = self.after(req_delay + self.t_ras);
                req_delay += self.t_rcd;
                bank.open_row = req.row;
            }
        }
        req_delay += self.t_ccd;
        bank.next_request = self.after(req_delay);
        req.respond(self.after(req_delay + self.t_cl));
    }

    fn make_request(&self, mem_req: ~MemRequest) -> Request {
        let addr = mem_req.addr;
        Request {
            channel: (addr >> DRAM_OFFSET_BITS) &
                     ((1u64 << self.channel_bits) - 1u64),
            bank: (addr >> self.bank_lsb) &
                  ((1u64 << self.bank_bits) - 1u64),
            row: addr >> self.row_lsb,
            mem_req: mem_req,
        }
    }

    fn request_conflict_state(&self, req: &Request) -> RequestConflictState {
        match self.channels[req.channel].banks[req.bank].open_row {
            NO_OPEN_ROW => Miss,
            row if row == req.row => Hit,
            _ => Conflict
        }
    }

    fn after(&self, cycles: i64) -> i64 {
        self.sim.now() + cycles * self.clock_divider
    }
}

impl<'self> MemRequestReceiver for Dram<'self> {
    fn receive_request(&mut self, mem_req: ~MemRequest) {
        let req = ~self.make_request(mem_req);
        self.channels[req.channel].waiting_reqs.push(req)
    }
}

impl Channel {
    fn new(num_banks: uint) -> Channel {
        Channel { banks: cons_vec(num_banks, Bank::new), waiting_reqs: ~[],
                  next_request: -1 }
    }
}

impl Bank {
    fn new() -> Bank {
        Bank { open_row: NO_OPEN_ROW, next_request: -1, next_conflict: -1 }
    }
}

impl Request {
    fn respond(&self, cycle: i64) { self.mem_req.respond(cycle) }
}
