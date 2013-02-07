package com.nixprime.nijikawa

import dram.SimpleDram
import mem_trace.{UsimmTraceReader, MemTraceCore}

object Nijikawa {
  val terminationCycle: Long = 100000000 // 100 million

  def main(args: Array[String]) {
    val sim = new Simulator
    val dram = new SimpleDram(sim)
    val core_trace_reader = new UsimmTraceReader("/home/jamiel/src/arch_sims/usimm/input/comm1")
    val core = new MemTraceCore(sim, core_trace_reader, dram.receiveRequest)

    core.superscalarWidth = 4
    core.robSize = 192

    core.init()
    dram.init()

    while (sim.now < terminationCycle) {
      core.tick()
      dram.tick()
      sim.now += 1
    }

    println(core.insnsRetired.toString + " instructions retired in " + terminationCycle.toString +
            " cycles")
  }
}
