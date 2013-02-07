#ifndef NIJIKAWA_MEM_REQUEST_H_
#define NIJIKAWA_MEM_REQUEST_H_

#include <functional>

#include "address.h"
#include "cycle.h"
#include "unique_ptr.h"

namespace nijikawa {

class MemResponse {
  public:
    MemResponse(Address addr) : addr_(addr) {}

    Address addr() const noexcept { return addr_; }

  private:
    Address addr_;
};

class MemResponseReceiver {
  public:
    virtual void receiveMemResponse(Cycle cycle,
        unique_ptr<MemResponse> response) = 0;
};

enum class MemRequestType {
  Read,
  Write,
};

class MemRequest {
  public:
    MemRequest(MemRequestType type, Address addr,
        MemResponseReceiver* response_receiver) : type_(type), addr_(addr),
        response_receiver_(response_receiver) {}

    MemRequestType type() const { return type_; }
    Address addr() const { return addr_; }

    void respond(Cycle cycle) const {
      if (response_receiver_) {
        response_receiver_->receiveMemResponse(cycle,
            make_unique<MemResponse>(addr_));
      }
    }

  private:
    MemRequestType type_;
    Address addr_;
    MemResponseReceiver* response_receiver_;
};

class MemRequestReceiver {
  public:
    virtual void receiveMemRequest(unique_ptr<MemRequest> request) = 0;
};

} // namespace nijikawa

#endif /* NIJIKAWA_MEM_REQUEST_H_ */
