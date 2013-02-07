#include <exception>
#include <iostream>

#include "simulator.h"
#include "core.h"
#include "dram.h"
#include "usimm_trace_reader.h"

namespace nijikawa {

constexpr Cycle kSimCycles = 100000000; // 100 million
const char* const kTraceFilename = "/home/jamiel/src/arch_sims/usimm/input/"
    "comm1";

int main() {
  Simulator sim;
  UsimmTraceReader trace_reader(kTraceFilename);
  Dram dram(sim);
  Core core(sim, &trace_reader, &dram);

  core.setSuperscalarWidth(4);
  core.setRobSize(192);

  core.init();
  dram.init();

  try {
    while (sim.now() < kSimCycles) {
      core.tick();
      dram.tick();
      sim.tick();
    }
  } catch (std::exception const& ex) {
    std::cout << ex.what() << std::endl;
  }

  std::cout << core.insnsRetired() << " instructions retired in " <<
      sim.now() << " cycles" << std::endl;

  return 0;
}

} // namespace nijikawa

int main() {
  return nijikawa::main();
}
