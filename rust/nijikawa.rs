extern mod extra;

use core::*;
use dram::*;
use mem_request::*;
use simulator::*;
use trace::*;
use usimm_trace_reader::*;

pub mod core;
pub mod dram;
pub mod mem_request;
pub mod simulator;
pub mod trace;
pub mod usimm_trace_reader;
pub mod util;

static sim_cycles: i64 = 100000000;
static trace_filename: &'static str = "/home/jamiel/src/usimm-v1.3/input/comm1";

fn main() {
    let mut sim = Simulator::new();
    let mut trace_reader = UsimmTraceReader::new(trace_filename);
    let mut dram = Dram::new(&sim, 1, 4);
    let mut core = @mut Core::new(&sim, &mut trace_reader as &mut TraceReader,
                                  &mut dram as &mut MemRequestReceiver, 4, 192);
    while sim.now() < sim_cycles {
        core.tick();
        dram.tick();
        sim.tick();
    }
    println!("{:u} instructions retired in {:i} cycles", core.insns_retired,
             sim.now())
}
