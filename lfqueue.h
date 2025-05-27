#pragma once

#include <vector>
#include <atomic>


namespace lf {

template <typename T> class lf_queue {

  // using unsigned to allow legal wrap around
  struct CellTy {
    std::atomic<unsigned> Sequence;
    T Data;
  };

  alignas(64) std::vector<CellTy> Buffer;
  alignas(64) unsigned BufferMask;
  alignas(64) std::atomic<unsigned> EnqueuePos, DequeuePos;

public:
  lf_queue(unsigned BufSize)
      : Buffer(BufSize), BufferMask(BufSize - 1) {
    if (BufSize > (1 << 30))
      throw std::runtime_error("buffer size too large");

    if (BufSize < 2)
      throw std::runtime_error("buffer size too small");

    if ((BufSize & (BufSize - 1)) != 0)
      throw std::runtime_error("buffer size is not power of 2");

    for (unsigned i = 0; i != BufSize; ++i)
      Buffer[i].Sequence.store(i, std::memory_order_relaxed);

    EnqueuePos.store(0, std::memory_order_relaxed);
    DequeuePos.store(0, std::memory_order_relaxed);
  }

  bool push(T Data) {
    CellTy *Cell;
    unsigned Pos;
    bool Res = false;

    while (!Res) {
      // fetch the current Position where to enqueue the item
      Pos = EnqueuePos.load(std::memory_order_relaxed);
      Cell = &Buffer[Pos & BufferMask];
      auto Seq = Cell->Sequence.load(std::memory_order_acquire);
      auto Diff = static_cast<int>(Seq) - static_cast<int>(Pos);

      // queue is full we cannot enqueue and just return false
      // another option: queue moved forward all way round
      if (Diff < 0)
        return false;

      // If its Sequence wasn't touched by other producers
      // check if we can increment the enqueue Position
      if (Diff == 0)
        Res = EnqueuePos.compare_exchange_weak(Pos, Pos + 1,
                                               std::memory_order_relaxed);
    }

    // write the item we want to enqueue and bump Sequence
    Cell->Data = std::move(Data);
    Cell->Sequence.store(Pos + 1, std::memory_order_release);
    return true;
  }

  bool pop(T &Data) {
    CellTy *Cell;
    unsigned Pos;
    bool Res = false;

    while (!Res) {
      // fetch the current Position from where we can dequeue an item
      Pos = DequeuePos.load(std::memory_order_relaxed);
      Cell = &Buffer[Pos & BufferMask];
      auto Seq = Cell->Sequence.load(std::memory_order_acquire);
      auto Diff = static_cast<int>(Seq) - static_cast<int>(Pos + 1);

      // probably the queue is empty, then return false
      if (Diff < 0)
        return false;

      // Check if its Sequence was changed by a producer and wasn't changed by
      // other consumers and if we can increment the dequeue Position
      if (Diff == 0)
        Res = DequeuePos.compare_exchange_weak(Pos, Pos + 1,
                                               std::memory_order_relaxed);
    }

    // read the item and update for the next round of the buffer
    Data = std::move(Cell->Data);
    Cell->Sequence.store(Pos + BufferMask + 1, std::memory_order_release);
    return true;
  }

  bool is_empty() const { return EnqueuePos.load() == DequeuePos.load(); }
};
}