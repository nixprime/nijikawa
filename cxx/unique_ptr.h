#ifndef NIJIKAWA_MEM_H_
#define NIJIKAWA_MEM_H_

#include <memory>

namespace nijikawa {

using std::unique_ptr;
using std::move;
using std::forward;

template <typename T, typename... Args>
inline unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(forward<Args>(args)...));
}

} // namespace nijikawa

#endif /* NIJIKAWA_MEM_H_ */
