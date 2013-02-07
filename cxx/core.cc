#include <iostream>

#include "core.h"

namespace nijikawa {

void Core::tick() {
  tickRetire();
  tickMem();
  tickIssue();
}

void Core::tickMem() {
  while (!waiting_responses_.empty()) {
    auto const& top = waiting_responses_.top();
    if (top.first > sim_.now()) {
      break;
    }
    receiveMemAddress(top.second);
    waiting_responses_.pop();
  }
}

void Core::tickIssue() {
  int remaining = superscalar_width_;
  while (remaining && (rob_insns_ < rob_size_)) {
    if (cur_mem_.prec) {
      // Preceding non-memory instruction
      rob_[rob_tail_] = sim_.now();
      cur_mem_.prec--;
    } else {
      // Memory instruction
      if (cur_mem_.is_write) {
        // Writes are non-blocking
        issueWrite(cur_mem_.addr);
        rob_[rob_tail_] = sim_.now();
      } else {
        auto& mshr = getMshr(cur_mem_.addr);
        mshr.rob_indices.push_back(rob_tail_);
        rob_[rob_tail_] = kMaxCycle;
        issueMshr(mshr);
      }
      cur_mem_ = trace_reader_->next();
    }
    remaining--;
    rob_insns_++;
    rob_tail_++;
    if (rob_tail_ >= rob_size_) {
      rob_tail_ = 0;
    }
  }
}

void Core::tickRetire() {
  int remaining = superscalar_width_;
  while (remaining && rob_insns_) {
    if (rob_[rob_head_] > sim_.now()) {
      break;
    }
    remaining--;
    rob_insns_--;
    rob_head_++;
    if (rob_head_ >= rob_size_) {
      rob_head_ = 0;
    }
    insns_retired_++;
  }
}

void Core::receiveMemAddress(Address addr) {
  auto it = mshrs_.find(addr);
  if (it == mshrs_.end()) {
    throw std::logic_error("Received unexpected memory response");
  }
  for (auto i : it->second.rob_indices) {
    rob_[i] = sim_.now();
  }
  mshrs_.erase(it);
}

auto Core::getMshr(Address const& addr) -> Mshr& {
  auto it = mshrs_.find(addr);
  if (it == mshrs_.end()) {
    std::tie(it, std::ignore) = mshrs_.insert(std::make_pair(addr,
        Mshr(addr)));
  }
  return it->second;
}

void Core::issueMshr(Mshr& mshr) {
  if (!mshr.issued) {
    mem_->receiveMemRequest(make_unique<MemRequest>(MemRequestType::Read,
        mshr.addr, this));
    mshr.issued = true;
  }
}

void Core::issueWrite(Address addr) {
  mem_->receiveMemRequest(make_unique<MemRequest>(MemRequestType::Write,
        addr, nullptr));
}

} // namespace nijikawa
