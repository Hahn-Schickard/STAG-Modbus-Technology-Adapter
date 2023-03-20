#include "Burst.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <set>

namespace Modbus_Technology_Adapter {

namespace Implementation {

struct BurstMaker { // Prepares one burst

  BurstMaker() = delete;

  // Lifetime of `readable` must include lifetime of `this`
  BurstMaker(RegisterSet const& readable, std::size_t max_burst_size)
      : readable_(readable), max_burst_size_(max_burst_size) {}

  void startBurst(RegisterIndex start_register) {
    start_register_ = start_register;
    burst_size_ = 0;
    limit_ = std::min( //
        readable_.endOfRange(start_register),
        (RegisterIndex)(start_register + max_burst_size_ - 1));
  }

  // Return value indicates whether the register did fit
  bool addRegister(RegisterIndex next_register) {
    bool fits = next_register <= limit_;
    if (fits)
      burst_size_ = next_register - start_register_ + 1;
    return fits;
  }

  std::size_t planNumber(
      RegisterIndex current_register /* precondition: in current burst */) {

    return total_size_ + (current_register - start_register_);
  }

  BurstPlan::Burst finishBurst() {
    total_size_ += burst_size_;
    return BurstPlan::Burst(start_register_, burst_size_);
  }

  std::size_t totalSize() { return total_size_; }

private:
  RegisterSet const& readable_;
  std::size_t max_burst_size_;
  RegisterIndex start_register_;
  std::size_t burst_size_;
  RegisterIndex limit_;
  std::size_t total_size_ = 0;
};

// A mutable version of `BurstPlan`.
// We need mutability only in the constructor.
struct MutableBurstPlan {

  std::vector<BurstPlan::Burst> bursts;
  std::size_t num_plan_registers = 0;
  std::vector<std::size_t> task_to_plan;

  MutableBurstPlan(BurstPlan::Task const& task, RegisterSet const& readable,
      std::size_t max_burst_size)
      : task_to_plan(task.size()) {

    // If the task is empty, there is nothing left to do
    if (task.empty())
      return;

    /*
      We use dynamic programming.
      For each register `r` from `task`, we compute the optimum plan for the
      subtask starting with `r`.
    */

    using ReverseTask = std::map<RegisterIndex, std::set<std::size_t>>;

    ReverseTask reverse_task;
    for (std::size_t i = 0; i < task.size(); ++i)
      reverse_task.try_emplace(task[i]).first->second.insert(i);
    // Now, `reverse_task[r]` holds all `i` such that `t[i] == r`.

    // We use iterators over `reverse_task` to iterate over registers.
    // Most of the time we do not care about the `->second` part of those.
    auto const& used_registers = reverse_task;
    using Forward = ReverseTask::const_iterator;
    using Backward = ReverseTask::const_reverse_iterator;

    /*
      The "plan" computed for a register `r` is not actually a `BurstPlan` or
      `MutableBurstPlan`. That would make the operations performed in the
      computation rather expensive. Instead, it is a linked list of
      `RegisterRange`s, where different lists may share nodes.
    */
    struct Node {
      RegisterRange range;
      std::shared_ptr<Node> next;
      Node(RegisterIndex start, RegisterIndex end,
          std::shared_ptr<Node> const& next_)
          : range(start,end), next(next_) {}
    };
    struct List {
      std::shared_ptr<Node> head;
      std::size_t length;
      std::size_t total_size;
      List(std::shared_ptr<Node> const& head_, //
          std::size_t length_, std::size_t total_size_)
          : head(head_), length(length_), total_size(total_size_) {}
    };
    std::map<RegisterIndex, List> optima;

    for (Backward i = used_registers.crbegin(); i != used_registers.crend();
        ++i) {
      // Invariant: `optima` has been populated for all registers after `r`.
      RegisterIndex r = i->first;

      // We loop through all potential bursts from `r`
      RegisterIndex limit = std::min( // the maximal potential range
          readable.endOfRange(r), //
          (RegisterIndex)(r + max_burst_size - 1));
      Forward next = i.base(); // forward and reverse iterators differ by 1
      Forward current = next;
      --current;
      // Now, `current` is a `Forward` version of `i`

      RegisterIndex best_end = current->first;
      auto best_next = next;
      std::size_t best_length = 1;
      std::size_t best_total = 1;
      if (next != used_registers.cend()) {
        auto const& optimum_for_next = optima.at(next->first);
        best_length += optimum_for_next.length;
        best_total += optimum_for_next.total_size;
      }
      // Now, the loop has been initialized

      while ((next != used_registers.cend()) && (next->first <= limit)) {
        current = next;
        ++next;
        std::size_t current_length = 1;
        std::size_t current_total = current->first - r + 1;
        if (next != used_registers.cend()) {
          auto const& optimum_for_next = optima.at(next->first);
          current_length += optimum_for_next.length;
          current_total += optimum_for_next.total_size;
        }

        if ((current_length < best_length) ||
            ((current_length == best_length) && (current_total < best_total))) {

          best_end = current->first;
          best_next = next;
          best_length = current_length;
          best_total = current_total;
        }
      }

      std::shared_ptr<Node> tail;
      if (best_next != used_registers.cend())
        tail = optima.at(best_next->first).head;
      auto node = std::make_shared<Node>(r, best_end, tail);
      optima.try_emplace(r, std::move(node), best_length, best_total);
    }

    // Now, dynamic programming is finished. We collect the result.

    auto current = reverse_task.cbegin();
    auto const& list = optima.at(current->first);
    for (std::shared_ptr<Node> node = list.head; node; node = node->next) {
      std::size_t size = node->range.end - node->range.begin + 1;
      bursts.emplace_back(node->range.begin, size);
      for (; (current != reverse_task.cend()) && (current->first <= node->range.end); ++current) {
        std::size_t plan_number =
            num_plan_registers + current->first - node->range.begin;
        for (auto j : current->second)
          task_to_plan[j] = plan_number;
      }
      num_plan_registers += size;
    }

/*
    BurstMaker maker(readable, max_burst_size);
    auto i = reverse_task.cbegin();

    if (i != reverse_task.cend()) {
      maker.startBurst(i->first);

      while (i != reverse_task.cend()) {
        if (maker.addRegister(i->first)) {
          std::size_t plan_number = maker.planNumber(i->first);
          auto const& indices = i->second;
          for (auto j : indices)
            task_to_plan[j] = plan_number;
          ++i;
        } else {
          bursts.push_back(maker.finishBurst());
          maker.startBurst(i->first);
        }
      }

      bursts.push_back(maker.finishBurst());
    }

    num_plan_registers = maker.totalSize();
*/

  }
};

} // namespace Implementation

BurstPlan::BurstPlan(Implementation::MutableBurstPlan&& source)
    : bursts(std::move(source.bursts)),
      num_plan_registers(source.num_plan_registers),
      task_to_plan(std::move(source.task_to_plan)) {}

BurstPlan::BurstPlan(
    Task const& task, RegisterSet const& readable, std::size_t max_burst_size)
    : BurstPlan(
          Implementation::MutableBurstPlan(task, readable, max_burst_size)) {}

BurstBuffer::BurstBuffer(BurstPlan::Task const& task,
    RegisterSet const& readable, std::size_t max_burst_size)
    : plan(task, readable, max_burst_size), padded(plan.num_plan_registers),
      compact(task.size()) {}

} // namespace Modbus_Technology_Adapter
