using System;
using System.Collections.Generic;

namespace Nijikawa
{
    class Core
    {
        private Simulator Sim { get; set; }
        private ITraceReader TraceReader { get; set; }
        private IMemRequestReceiver Mem { get; set; }

        public int SuperscalarWidth { get; set; }
        public int RobSize { get; set; }

        private int RobHead { get; set; }
        private int RobTail { get; set; }
        private int RobInsns { get; set; }
        private long[] Rob { get; set; }
        private TraceRecord CurMem { get; set; }
        private Dictionary<ulong, Mshr> Mshrs { get; set; }
        private PriorityQueue<QueuedResponse> WaitingResponses { get; set; }

        public ulong InsnsRetired { get; private set; }

        private class Mshr
        {
            public ulong Addr { get; private set; }
            public bool Issued { get; set; }
            public List<int> RobIndices { get; private set; }

            public Mshr(ulong addr)
            {
                Addr = addr;
                Issued = false;
                RobIndices = new List<int>();
            }
        }

        private class QueuedResponse : IComparable<QueuedResponse>
        {
            public long Cycle { get; private set; }
            public MemResponse Response { get; private set; }

            public QueuedResponse(long cycle, MemResponse response)
            {
                Cycle = cycle;
                Response = response;
            }

            int IComparable<QueuedResponse>.CompareTo(QueuedResponse other)
            {
                // Negated since PriorityQueue is a max-prioq
                return -Cycle.CompareTo(other.Cycle);
            }
        }

        public Core(Simulator sim, ITraceReader traceReader,
                IMemRequestReceiver mem, int superscalarWidth, int robSize)
        {
            Sim = sim;
            TraceReader = traceReader;
            Mem = mem;
            SuperscalarWidth = superscalarWidth;
            RobSize = robSize;

            RobHead = 0;
            RobTail = 0;
            RobInsns = 0;
            Rob = new long[RobSize];
            CurMem = TraceReader.NextTraceRecord();
            Mshrs = new Dictionary<ulong, Mshr>();
            WaitingResponses = new PriorityQueue<QueuedResponse>();
        }

        public void Tick()
        {
            TickRetire();
            TickMem();
            TickIssue();
        }

        private void TickIssue()
        {
            int remaining = SuperscalarWidth;
            while (remaining > 0 && RobInsns < RobSize)
            {
                if (CurMem.Prec > 0)
                {
                    Rob[RobTail] = Sim.Now;
                    CurMem.Prec--;
                }
                else
                {
                    if (CurMem.IsWrite)
                    {
                        IssueWrite(CurMem.Addr);
                        Rob[RobTail] = Sim.Now;
                    }
                    else
                    {
                        var mshr = GetMshr(CurMem.Addr);
                        mshr.RobIndices.Add(RobTail);
                        Rob[RobTail] = long.MaxValue;
                        IssueMshr(mshr);
                    }
                    CurMem = TraceReader.NextTraceRecord();
                }
                remaining--;
                RobInsns++;
                RobTail++;
                if (RobTail >= RobSize)
                {
                    RobTail = 0;
                }
            }
        }

        private void TickMem()
        {
            while (WaitingResponses.Count > 0)
            {
                var responsePair = WaitingResponses.Top();
                if (responsePair.Cycle > Sim.Now)
                {
                    break;
                }
                DeliverMemResponse(responsePair.Response);
                WaitingResponses.Remove();
            }
        }

        private void TickRetire()
        {
            int remaining = SuperscalarWidth;
            while (remaining > 0 && RobInsns > 0)
            {
                if (Rob[RobHead] > Sim.Now)
                {
                    break;
                }
                remaining--;
                RobInsns--;
                RobHead++;
                if (RobHead >= RobSize)
                {
                    RobHead = 0;
                }
                InsnsRetired++;
            }
        }

        private void ReceiveMemResponse(long cycle, MemResponse response)
        {
            WaitingResponses.Add(new QueuedResponse(cycle, response));
        }

        private void DeliverMemResponse(MemResponse response)
        {
            foreach (var i in Mshrs[response.Addr].RobIndices)
            {
                Rob[i] = Sim.Now;
            }
            Mshrs.Remove(response.Addr);
        }

        private Mshr GetMshr(ulong addr)
        {
            if (!Mshrs.ContainsKey(addr))
            {
                Mshrs.Add(addr, new Mshr(addr));
            }
            return Mshrs[addr];
        }

        private void IssueMshr(Mshr mshr)
        {
            if (!mshr.Issued)
            {
                Mem.ReceiveMemRequest(new MemRequest(MemRequestType.Read,
                            mshr.Addr, ReceiveMemResponse));
                mshr.Issued = true;
            }
        }

        private void IssueWrite(ulong addr)
        {
            Mem.ReceiveMemRequest(new MemRequest(MemRequestType.Write, addr,
                        null));
        }
    }
}
