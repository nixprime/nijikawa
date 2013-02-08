#!/usr/bin/env python
# -*- coding: utf-8 -*-

import heapq

# Python's long is a bigint...
MAX_CYCLE = 9223372036854775807L

class Simulator(object):

    def __init__(self):
        self._now = 0

    def now(self):
        return self._now

    def tick(self):
        self._now += 1

class MemRequestType(object):

    READ = 0
    WRITE = 1

class MemRequest(object):

    def __init__(self, type, addr, response_receiver):
        self.type = type
        self.addr = addr
        self.response_receiver = response_receiver

    def respond(self, cycle):
        if self.response_receiver:
            self.response_receiver.receive_mem_response(cycle,
                                                        MemResponse(self.addr))

class MemResponse(object):

    def __init__(self, addr):
        self.addr = addr

class TraceRecord(object):

    def __init__(self, addr, prec, is_write):
        self.addr = addr
        self.prec = prec
        self.is_write = is_write

class RequestConflictState(object):

    HIT = 0
    MISS = -1
    CONFLICT = 1

class Dram(object):

    OFFSET_BITS = 6
    ROW_SIZE_BITS = 13
    NO_OPEN_ROW = -1

    class Request(object):

        def __init__(self, dram, mem_req):
            self.mem_req = mem_req
            self.channel = dram.map_channel(mem_req.addr)
            self.bank = dram.map_bank(mem_req.addr)
            self.row = dram.map_row(mem_req.addr)

        def respond(self, cycle):
            self.mem_req.respond(cycle)

    class BankState(object):

        def __init__(self):
            self.open_row = Dram.NO_OPEN_ROW
            self.next_request = -1
            self.next_conflict = -1

    class ChannelState(object):

        def __init__(self, num_banks=0):
            self.waiting_reqs = []
            self.banks = [Dram.BankState() for i in xrange(num_banks)]
            self.next_request = -1

    def __init__(self, sim, channel_bits=1, bank_bits=4):
        self.sim = sim
        self.channel_bits = channel_bits
        self.bank_bits = bank_bits
        self.bank_lsb = Dram.ROW_SIZE_BITS + channel_bits
        self.row_lsb = self.bank_lsb + bank_bits
        self.channels = [Dram.ChannelState(1 << bank_bits)
                         for i in xrange(1 << channel_bits)]
        self.clock_div = 4
        self.t_ccd = 4
        self.t_cl = 11
        self.t_rcd = 11
        self.t_rp = 11
        self.t_ras = 28

    def map_channel(self, addr):
        return (addr >> Dram.OFFSET_BITS) & ((1 << self.channel_bits) - 1)

    def map_bank(self, addr):
        return (addr >> self.bank_lsb) & ((1 << self.bank_bits) - 1)

    def map_row(self, addr):
        return addr >> self.row_lsb

    def receive_mem_request(self, mem_req):
        req = Dram.Request(self, mem_req)
        self.channels[req.channel].waiting_reqs.append(req)

    def tick(self):
        if self.sim.now() % self.clock_div != 0:
            return
        for chan in self.channels:
            if chan.next_request <= self.sim.now():
                req = self.best_request(chan)
                if req:
                    self.issue_request(req)

    def best_request(self, chan):
        best_i = -1
        for i in xrange(len(chan.waiting_reqs)):
            req = chan.waiting_reqs[i]
            if chan.banks[req.bank].next_request > self.sim.now():
                continue
            state = self.request_conflict_state(req)
            if state == RequestConflictState.HIT:
                best_i = i
                break
            if best_i < 0:
                if state == RequestConflictState.CONFLICT and \
                        chan.banks[req.bank].next_conflict > self.sim.now():
                    continue;
                best_i = i
        if best_i >= 0:
            best_req = chan.waiting_reqs[best_i]
            del chan.waiting_reqs[best_i]
            return best_req
        else:
            return None

    def issue_request(self, req):
        chan = self.channels[req.channel]
        bank = chan.banks[req.bank]
        state = self.request_conflict_state(req)
        req_delay = 0
        chan.next_request = self.after(self.t_ccd)
        if state != RequestConflictState.HIT:
            if state == RequestConflictState.CONFLICT:
                req_delay += self.t_rp
            bank.next_conflict = self.after(req_delay + self.t_ras)
            req_delay += self.t_rcd
            bank.open_row = req.row
        req_delay += self.t_ccd
        bank.next_request = self.after(req_delay)
        req.respond(self.after(req_delay + self.t_cl))

    def request_conflict_state(self, req):
        bank = self.channels[req.channel].banks[req.bank]
        if bank.open_row == req.row:
            return RequestConflictState.HIT
        elif bank.open_row == Dram.NO_OPEN_ROW:
            return RequestConflictState.MISS
        else:
            return RequestConflictState.CONFLICT

    def after(self, component_cycles):
        return self.sim.now() + component_cycles * self.clock_div

class Core(object):

    class Mshr(object):

        def __init__(self, addr):
            self.addr = addr
            self.rob_indices = []
            self.issued = False

    def __init__(self, sim, trace_reader, mem, superscalar_width, rob_size):
        self.sim = sim
        self.trace_reader = trace_reader
        self.mem = mem
        self.superscalar_width = superscalar_width
        self.rob_size = rob_size
        self.rob_head = 0
        self.rob_tail = 0
        self.rob_insns = 0
        self.rob = [0L] * rob_size
        self.mshrs = {}
        self.cur_mem = trace_reader.next()
        self.waiting_responses = []
        self.insns_retired = 0

    def receive_mem_response(self, cycle, response):
        heapq.heappush(self.waiting_responses, (cycle, response))

    def tick(self):
        self.tick_retire()
        self.tick_mem()
        self.tick_issue()

    def tick_retire(self):
        remaining = self.superscalar_width
        while remaining and self.rob_insns:
            if self.rob[self.rob_head] > self.sim.now():
                break
            remaining -= 1
            self.rob_insns -= 1
            self.rob_head += 1
            if self.rob_head >= self.rob_size:
                self.rob_head = 0
            self.insns_retired += 1

    def tick_mem(self):
        while self.waiting_responses and \
                self.waiting_responses[0][0] <= self.sim.now():
            self.deliver_mem_response(self.waiting_responses[0][1])
            heapq.heappop(self.waiting_responses)

    def deliver_mem_response(self, response):
        mshr = self.mshrs[response.addr]
        for i in mshr.rob_indices:
            self.rob[i] = self.sim.now()
        del self.mshrs[response.addr]

    def tick_issue(self):
        remaining = self.superscalar_width
        while remaining and self.rob_insns < self.rob_size:
            if self.cur_mem.prec:
                self.rob[self.rob_tail] = self.sim.now()
                self.cur_mem.prec -= 1
            else:
                if self.cur_mem.is_write:
                    self.issue_write(self.cur_mem.addr)
                    self.rob[self.rob_tail] = self.sim.now()
                else:
                    mshr = self.get_mshr(self.cur_mem.addr)
                    mshr.rob_indices.append(self.rob_tail)
                    self.rob[self.rob_tail] = MAX_CYCLE
                    self.issue_mshr(mshr)
                self.cur_mem = trace_reader.next()
            remaining -= 1
            self.rob_insns += 1
            self.rob_tail += 1
            if self.rob_tail >= self.rob_size:
                self.rob_tail = 0

    def get_mshr(self, addr):
        if addr not in self.mshrs:
            self.mshrs[addr] = Core.Mshr(addr)
        return self.mshrs[addr]

    def issue_mshr(self, mshr):
        if not mshr.issued:
            self.mem.receive_mem_request(MemRequest(MemRequestType.READ,
                                                    mshr.addr, self))
            mshr.issued = True

    def issue_write(self, addr):
        self.mem.receive_mem_request(MemRequest(MemRequestType.WRITE, addr,
                                                None))

class UsimmTraceReader(object):

    def __init__(self, file):
        self.file = file

    def next(self):
        prec, type_c, addr = self.file.readline().split()[:3]
        if type_c == "R":
            is_write = False
        elif type_c == "W":
            is_write = True
        else:
            raise Exception("Unknown request type " + type_c + " in trace")
        return TraceRecord(long(addr, 0), int(prec, 0), is_write)

if __name__ == "__main__":
    TRACE_FILENAME = "/home/jamiel/src/arch_sims/usimm/input/comm1"
    SIM_CYCLES = 100000000L
    sim = Simulator()
    with open(TRACE_FILENAME, 'r') as trace_file:
        trace_reader = UsimmTraceReader(trace_file)
        dram = Dram(sim, channel_bits=1, bank_bits=4)
        core = Core(sim, trace_reader, dram, superscalar_width=4,
                    rob_size=192)
        while sim.now() < SIM_CYCLES:
            core.tick()
            dram.tick()
            sim.tick()
        print core.insns_retired, "instructions retired in", SIM_CYCLES, \
                "cycles"

