#ifndef NIJIKAWA_DRAM_H_
#define NIJIKAWA_DRAM_H_

#include <vector>

#include "address.h"
#include "cycle.h"
#include "mem_request.h"
#include "simulator.h"
#include "unique_ptr.h"

namespace nijikawa {

class Dram : public MemRequestReceiver {
  public:
    explicit Dram(Simulator const& sim) : sim_(sim) {}

    void init();
    void tick();

    virtual void receiveMemRequest(unique_ptr<MemRequest> mem_req) override {
      // Wrap the MemRequest in a Dram::Request and insert it into the
      // appropriate channel's waiting request queue.
      auto req = make_unique<Request>(*this, move(mem_req));
      channels_[req->channel].waiting_reqs.push_back(move(req));
    }

    // Address mapping: extract DRAM geometric indices from physical address
    Address mapChannel(Address addr) const noexcept {
      return (addr >> channel_lsb_) & ((Address(1) << channel_bits_) - 1);
    }
    Address mapBank(Address addr) const noexcept {
      return (addr >> bank_lsb_) & ((Address(1) << bank_bits_) - 1);
    }
    Address mapRow(Address addr) const noexcept {
      return addr >> row_lsb_;
    }

  private:
    Simulator const& sim_;

    // Address mapping (row:bank:channel:offset)
    int channel_bits_ = 1; // 2 channels
    int channel_lsb_ = 6;
    int bank_bits_ = 4; // 16 banks/channel
    int bank_lsb_ = 13;
    int row_lsb_ = 17;

    // Timing constraints
    Cycle clock_div_ = 4; // Ratio between DRAM and simulator clocks
    Cycle t_ccd_ = 4; // Latency from RD/WR to RD/WR (channel)
    Cycle t_cl_ = 11; // Latency from RD/WR to beginning of data
    Cycle t_rcd_ = 11; // Latency from ACT to RD/WR (bank)
    Cycle t_rp_ = 11; // Latency from PRE to ACT (bank)
    Cycle t_ras_ = 28; // Latency from ACT to PRE (bank)

    enum class RequestConflictState {
      Hit,
      Miss,
      Conflict,
    };

    struct Request {
      Address channel;
      Address bank;
      Address row;
      unique_ptr<MemRequest> mem_req;

      explicit Request(Dram const& dram, unique_ptr<MemRequest> mem_req) :
          channel(dram.mapChannel(mem_req->addr())),
          bank(dram.mapBank(mem_req->addr())),
          row(dram.mapRow(mem_req->addr())),
          mem_req(move(mem_req)) {}

      void respond(Cycle cycle) const { mem_req->respond(cycle); }
    };

    static constexpr Address kNoOpenRow = ~Address(0);

    struct BankState {
      Address open_row = kNoOpenRow;
      /** The next cycle in which this bank can receive a new request. */
      Cycle next_request = 0;
      /** The next cycle in which this bank can receive a conflict request. */
      Cycle next_conflict = 0;
    };

    struct ChannelState {
      std::vector<unique_ptr<Request>> waiting_reqs;
      std::vector<BankState> banks;
      Cycle next_request = 0;
    };

    std::vector<ChannelState> channels_;

    void tickChannel(ChannelState& chan);

    // Choose up to one request from the given channel's waiting request queue
    // to be issued, remove it from the waiting request queue, and return it.
    // If no request should be issued, return null.
    unique_ptr<Request> bestRequest(ChannelState& chan);

    // Issue the given request.
    void issueRequest(unique_ptr<Request> req_ptr);

    RequestConflictState requestConflictState(Request const& req) const;
};

} // namespace nijikawa

#endif /* NIJIKAWA_DRAM_H_ */
