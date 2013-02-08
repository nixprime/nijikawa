#ifndef NIJIKAWA_CORE_H_
#define NIJIKAWA_CORE_H_

#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "mem_request.h"
#include "simulator.h"
#include "trace_reader.h"
#include "unique_ptr.h"

namespace nijikawa {

class Core : public MemResponseReceiver {
  public:
    Core(Simulator const& sim, TraceReader* trace_reader,
        MemRequestReceiver* mem, int superscalar_width, int rob_size) :
        sim_(sim), trace_reader_(trace_reader), mem_(mem),
        superscalar_width_(superscalar_width), rob_size_(rob_size),
        rob_(rob_size, 0) {
      cur_mem_ = trace_reader_->next();
    }

    void tick();

    virtual void receiveMemResponse(Cycle cycle,
        unique_ptr<MemResponse> mem_resp_ptr) override {
      waiting_responses_.push(std::make_pair(cycle, mem_resp_ptr->addr()));
    }

    std::uint64_t insnsRetired() const noexcept { return insns_retired_; }

  private:
    Simulator const& sim_;
    TraceReader* trace_reader_;
    MemRequestReceiver* mem_;

    // Parameters
    int superscalar_width_ = 1;
    int rob_size_ = 1;

    // State
    int rob_head_ = 0;
    int rob_tail_ = 0;
    int rob_insns_ = 0;
    TraceRecord cur_mem_;
    /**
     * ROB structure. Each element contains the earliest cycle in which the
     * corresponding instruction can be retired.
     */
    std::vector<Cycle> rob_;

    // Miss status handling register.
    struct Mshr {
      Address addr;
      bool issued = false;
      std::vector<int> rob_indices;
      explicit Mshr(Address addr) : addr(addr) {}
    };

    std::unordered_map<Address, Mshr> mshrs_;

    // Storing unique_ptrs in a std::priority_queue is impractical, because
    // there is no version of std::priority_queue::top() that returns a
    // non-const reference. This is a pretty significant defect in
    // std::priority_queue that will hopefully be fixed in C++1x.
    typedef std::pair<Cycle, Address> QueuedResponse;

    struct QueuedResponseComparator {
      bool operator()(QueuedResponse const& x, QueuedResponse const& y) const {
        // Normally std::priority_queue is a max-prioq, and the comparator
        // returns true if x<y. We want a min-prioq, so the comparator should
        // return true if x>y.
        return x.first > y.first;
      }
    };

    // The choice to have std::priority_queue's template parameters in the
    // order <T, Container, Compare> rather than <T, Compare, Container> is...
    // questionable.
    std::priority_queue<QueuedResponse, std::vector<QueuedResponse>,
        QueuedResponseComparator> waiting_responses_;

    std::uint64_t insns_retired_ = 0;

    void tickMem();
    void tickIssue();
    void tickRetire();
    void receiveMemAddress(Address addr);

    /**
     * Get a MSHR for a read request to the given address. If no MSHR already
     * exists, allocate one.
     */
    Mshr& getMshr(Address const& addr);

    /** If the given MSHR's read request has not yet been issued, issue it. */
    void issueMshr(Mshr& mshr);

    /** Issue a write request to the given address. */
    void issueWrite(Address addr);
};

} // namespace nijikawa

#endif /* NIJIKAWA_CORE_H_ */
