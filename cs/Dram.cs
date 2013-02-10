using System;
using System.Collections.Generic;

namespace Nijikawa
{
    class Dram : IMemRequestReceiver
    {
        public const int OffsetBits = 6;
        public const int RowSizeBits = 13;

        private const ulong NoOpenRow = ~0UL;

        private Simulator Sim { get; set; }

        private int ChannelBits { get; set; }
        private int BankBits { get; set; }
        private int BankLsb { get; set; }
        private int RowLsb { get; set; }

        public long ClockDivider { get; set; }
        public long tCCD { get; set; }
        public long tCL { get; set; }
        public long tRCD { get; set; }
        public long tRP { get; set; }
        public long tRAS { get; set; }

        private ChannelState[] Channels { get; set; }

        private enum RequestConflictState
        {
            Hit,
            Miss,
            Conflict,
        }

        private class Request
        {
            public ulong Channel { get; private set; }
            public ulong Bank { get; private set; }
            public ulong Row { get; private set; }
            public MemRequest MemReq { get; private set; }

            public Request(Dram dram, MemRequest memReq)
            {
                Channel = dram.MapChannel(memReq.Addr);
                Bank = dram.MapBank(memReq.Addr);
                Row = dram.MapRow(memReq.Addr);
                MemReq = memReq;
            }

            public void Respond(long cycle)
            {
                MemReq.Respond(cycle);
            }
        }

        private class BankState
        {
            public ulong OpenRow { get; set; }
            public long NextRequest { get; set; }
            public long NextConflict { get; set; }

            public BankState()
            {
                OpenRow = NoOpenRow;
                NextRequest = -1;
                NextConflict = -1;
            }
        }

        private class ChannelState
        {
            public List<Request> WaitingReqs { get; private set; }
            public BankState[] Banks { get; private set; }
            public long NextRequest { get; set; }

            public ChannelState(int numBanks)
            {
                WaitingReqs = new List<Request>();
                Banks = new BankState[numBanks];
                for (int i = 0; i < Banks.Length; i++)
                {
                    Banks[i] = new BankState();
                }
                NextRequest = -1;
            }
        }

        public Dram(Simulator sim, int channelBits, int bankBits)
        {
            Sim = sim;

            ChannelBits = channelBits;
            BankBits = bankBits;
            BankLsb = RowSizeBits + channelBits;
            RowLsb = BankLsb + bankBits;

            ClockDivider = 4;
            tCCD = 4;
            tCL = 11;
            tRCD = 11;
            tRP = 11;
            tRAS = 28;

            int numChannels = 1 << ChannelBits;
            int numBanksPerChannel = 1 << BankBits;
            Channels = new ChannelState[numChannels];
            for (int i = 0; i < Channels.Length; i++)
            {
                Channels[i] = new ChannelState(numBanksPerChannel);
            }
        }

        void IMemRequestReceiver.ReceiveMemRequest(MemRequest memReq)
        {
            var req = new Request(this, memReq);
            Channels[req.Channel].WaitingReqs.Add(req);
        }

        public void Tick()
        {
            if (Sim.Now % ClockDivider != 0)
            {
                return;
            }
            foreach (var chan in Channels)
            {
                if (chan.NextRequest <= Sim.Now)
                {
                    var req = BestRequest(chan);
                    if (req != null)
                    {
                        IssueRequest(req);
                    }
                }
            }
        }

        public ulong MapChannel(ulong addr)
        {
            return (addr >> OffsetBits) & ((1UL << ChannelBits) - 1UL);
        }

        public ulong MapBank(ulong addr)
        {
            return (addr >> BankLsb) & ((1UL << BankBits) - 1UL);
        }

        public ulong MapRow(ulong addr)
        {
            return addr >> RowLsb;
        }

        private Request BestRequest(ChannelState chan)
        {
            int best = -1;
            for (int i = 0; i < chan.WaitingReqs.Count; i++)
            {
                var req = chan.WaitingReqs[i];
                if (chan.Banks[req.Bank].NextRequest > Sim.Now)
                {
                    continue;
                }
                var state = ConflictState(req);
                if (state == RequestConflictState.Hit)
                {
                    best = i;
                    break;
                }
                if (best < 0)
                {
                    if (state == RequestConflictState.Conflict &&
                            chan.Banks[req.Bank].NextConflict > Sim.Now)
                    {
                        continue;
                    }
                    best = i;
                }
            }
            if (best >= 0)
            {
                var req = chan.WaitingReqs[best];
                chan.WaitingReqs.RemoveAt(best);
                return req;
            }
            return null;
        }

        private void IssueRequest(Request req)
        {
            var chan = Channels[req.Channel];
            var bank = chan.Banks[req.Bank];
            var state = ConflictState(req);
            long reqDelay = 0;
            chan.NextRequest = After(tCCD);
            if (state != RequestConflictState.Hit)
            {
                if (state == RequestConflictState.Conflict)
                {
                    reqDelay += tRP;
                }
                bank.NextConflict = After(reqDelay + tRAS);
                reqDelay += tRCD;
                bank.OpenRow = req.Row;
            }
            reqDelay += tCCD;
            bank.NextRequest = After(reqDelay);
            req.Respond(After(reqDelay + tCL));
        }

        private RequestConflictState ConflictState(Request req)
        {
            var bank = Channels[req.Channel].Banks[req.Bank];
            if (bank.OpenRow == req.Row)
            {
                return RequestConflictState.Hit;
            }
            else if (bank.OpenRow == NoOpenRow)
            {
                return RequestConflictState.Miss;
            }
            else
            {
                return RequestConflictState.Conflict;
            }
        }

        private long After(long componentCycles)
        {
            return Sim.Now + componentCycles * ClockDivider;
        }
    }
}
