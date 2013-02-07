#ifndef NIJIKAWA_SIMULATOR_H_
#define NIJIKAWA_SIMULATOR_H_

#include "cycle.h"

namespace nijikawa {

class Simulator {
  public:
    Cycle now() const noexcept { return now_; }

    void tick() { now_++; }

  private:
    Cycle now_ = 0;
};

} // namespace nijikawa

#endif /* NIJIKAWA_SIMULATOR_H_ */
