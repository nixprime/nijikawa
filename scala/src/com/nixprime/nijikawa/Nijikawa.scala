package com.nixprime.nijikawa

import component.{Dram, UsimmTraceReader, Core}

object Nijikawa {
  val terminationCycle: Long = 100000000 // 100 million

  def main(args: Array[String]) {
    val sim = new Simulator
    val dram = new Dram(sim, 1, 4)
    val core_trace_reader = new UsimmTraceReader("/home/jamiel/src/arch_sims/usimm/input/comm1")
    val core = new Core(sim, core_trace_reader, dram.receiveRequest, 4, 192)

    while (sim.now < terminationCycle) {
      core.tick()
      dram.tick()
      sim.now += 1
    }

    println(core.insnsRetired.toString + " instructions retired in " + terminationCycle.toString +
            " cycles")
  }
}
