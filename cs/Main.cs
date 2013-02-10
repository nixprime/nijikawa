using System;

namespace Nijikawa
{
    class Nijikawa
    {
        public const long SimCycles = 100000000;
        public const string TraceFilename =
                "/home/jamiel/src/arch_sims/usimm/input/comm1";

        static void Main(string[] args)
        {
            var sim = new Simulator();
            var traceReader = new UsimmTraceReader(TraceFilename);
            var dram = new Dram(sim, 1, 4);
            var core = new Core(sim, traceReader, dram, 4, 192);

            while (sim.Now < SimCycles)
            {
                core.Tick();
                dram.Tick();
                sim.Tick();
            }

            Console.WriteLine(core.InsnsRetired.ToString() + " instructions " +
                    "retired in " + sim.Now.ToString() + " cycles");
        }
    }
}
