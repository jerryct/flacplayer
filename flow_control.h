// SPDX-License-Identifier: MIT

#ifndef FLOW_CONTROL_H
#define FLOW_CONTROL_H

#include <condition_variable>
#include <mutex>

namespace plac {

class FlowControl {
public:
  FlowControl() : m_{}, cv_{} {}

  void Await() { // acquire
    std::unique_lock<std::mutex> lk(m_);
    cv_.wait(lk);
  }

  void Notify() { // release
    cv_.notify_one();
  }

private:
  std::mutex m_;
  std::condition_variable cv_;
};

} // namespace plac

#endif
