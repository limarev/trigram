#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>

namespace ts {

template <typename T> T Limiter();

template <typename T> class TSQueue {
private:
  std::queue<T> q_;
  mutable std::mutex mut_;
  std::condition_variable cond_;

public:
  void push(T value) {
    std::lock_guard lk{mut_};
    q_.push(std::move(value));
    cond_.notify_one();
  }

  bool wait_and_pop(T &value) {
    std::unique_lock lk{mut_};
    cond_.wait(lk, [this] { return !q_.empty(); });
    auto &&task = q_.front();
    if (task == Limiter<T>()) {
      q_.pop();
      return false;
    }

    std::swap(value, task);
    q_.pop();

    return true;
  }
};
}