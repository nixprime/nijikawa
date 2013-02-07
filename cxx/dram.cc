#include "dram.h"

namespace nijikawa {

void Dram::tick() {
  if (sim_.now() % clock_div_ != 0) {
    return;
  }
  for (auto& chan : channels_) {
    if (chan.next_request <= sim_.now()) {
      auto req = bestRequest(chan);
      if (req) {
        issueRequest(move(req));
      }
    }
  }
}

auto Dram::bestRequest(ChannelState& chan) -> unique_ptr<Request> {
  auto end = chan.waiting_reqs.end();
  auto best_req_it = end;
  for (auto it = chan.waiting_reqs.begin(); it != end; ++it) {
    auto& req = **it;
    // Filter out requests that can't be issued due to bank request delay
    if (chan.banks[req.bank].next_request > sim_.now()) {
      continue;
    }
    auto req_state = requestConflictState(req);
    // If the request is the first row hit in the queue, it's the best request
    if (req_state == RequestConflictState::Hit) {
      best_req_it = it;
      break;
    }
    // If we haven't seen another request yet, and this one is schedulable,
    // it's the best request so far
    if (best_req_it == end) {
      if (req_state == RequestConflictState::Conflict &&
          chan.banks[req.bank].next_conflict > sim_.now()) {
        continue;
      }
      best_req_it = it;
    }
  }
  if (best_req_it != end) {
    auto best_req = move(*best_req_it);
    chan.waiting_reqs.erase(best_req_it);
    return best_req;
  } else {
    return nullptr;
  }
}

void Dram::issueRequest(unique_ptr<Request> req_ptr) {
  auto const& req = *req_ptr;
  auto& chan = channels_[req.channel];
  auto& bank = chan.banks[req.bank];
  auto state = requestConflictState(req);
  Cycle req_delay = 0;
  // Note that while this models *average* channel bandwidth over time, it does
  // not model *instantaneous* channel bandwidth in the sense that two requests
  // with different conflict states might have their commands or data overlap.
  // We accept this as a simplifying approximation.
  chan.next_request = sim_.now() + t_ccd_ * clock_div_;
  if (state != RequestConflictState::Hit) {
    if (state == RequestConflictState::Conflict) {
      // Precharge
      req_delay += t_rp_;
    }
    // Activate
    bank.next_conflict = sim_.now() + (req_delay + t_ras_) * clock_div_;
    req_delay += t_rcd_;
    bank.open_row = req.row;
  }
  // Read/write
  req_delay += t_ccd_;
  bank.next_request = sim_.now() + req_delay * clock_div_;
  Cycle data_delay = req_delay + t_cl_;
  req.respond(sim_.now() + data_delay * clock_div_);
}

auto Dram::requestConflictState(Request const& req) const ->
    RequestConflictState {
  auto const& bank = channels_[req.channel].banks[req.bank];
  if (bank.open_row == req.row) {
    return RequestConflictState::Hit;
  } else if (bank.open_row == kNoOpenRow) {
    return RequestConflictState::Miss;
  } else {
    return RequestConflictState::Conflict;
  }
}

} // namespace nijikawa
