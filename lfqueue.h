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

  std::vector<CellTy> Buffer;
  unsigned BufferMask;
  std::atomic<unsigned> EnqueuePos, DequeuePos;

public:
  lf_queue(unsigned BufSize) : Buffer(BufSize), BufferMask(BufSize - 1) {
    if (BufSize > (1 << 30))
      throw std::runtime_error("buffer size too large");

    if (BufSize < 2)
      throw std::runtime_error("buffer size too small");

    if ((BufSize & (BufSize - 1)) != 0)
      throw std::runtime_error("buffer size is not power of 2");

    for (unsigned i = 0; i != BufSize; ++i)
      Buffer[i].Sequence.store(i);

    EnqueuePos.store(0);
    DequeuePos.store(0);
  }

  bool push(T Data) {
    CellTy *Cell;
    unsigned Pos;
    bool Res = false;

    while (!Res) {
      // fetch the current Position where to enqueue the item
      Pos = EnqueuePos.load();
      Cell = &Buffer[Pos & BufferMask];
      auto Seq = Cell->Sequence.load();
      auto Diff = static_cast<int>(Seq) - static_cast<int>(Pos);

#ifdef LOG
      {
        std::lock_guard<std::mutex> Lk(LogMut);
        std::cout << "push: ";
        std::cout << Pos << " " << Cell << " " << Seq << " " << Diff
                  << std::endl;
      }
#endif

      // queue is full we cannot enqueue and just return false
      // another option: queue moved forward all way round
      if (Diff < 0)
        return false;

      // If its Sequence wasn't touched by other producers
      // check if we can increment the enqueue Position
      if (Diff == 0)
        Res = EnqueuePos.compare_exchange_weak(Pos, Pos + 1);
    }

    // write the item we want to enqueue and bump Sequence
    Cell->Data = std::move(Data);
    Cell->Sequence.store(Pos + 1);
    return true;
  }

  bool pop(T &Data) {
    CellTy *Cell;
    unsigned Pos;
    bool Res = false;

    while (!Res) {
      // fetch the current Position from where we can dequeue an item
      Pos = DequeuePos.load();
      Cell = &Buffer[Pos & BufferMask];
      auto Seq = Cell->Sequence.load();
      auto Diff = static_cast<int>(Seq) - static_cast<int>(Pos + 1);

#ifdef LOG
      {
        std::lock_guard<std::mutex> Lk(LogMut);
        std::cout << "pop: ";
        std::cout << Pos << " " << Cell << " " << Seq << " " << Diff
                  << std::endl;
      }
#endif

      // probably the queue is empty, then return false
      if (Diff < 0)
        return false;

      // Check if its Sequence was changed by a producer and wasn't changed by
      // other consumers and if we can increment the dequeue Position
      if (Diff == 0)
        Res = DequeuePos.compare_exchange_weak(Pos, Pos + 1);
    }

    // read the item and update for the next round of the buffer
    Data = std::move(Cell->Data);
    Cell->Sequence.store(Pos + BufferMask + 1);
    return true;
  }

  bool is_empty() const { return EnqueuePos.load() == DequeuePos.load(); }
};
}