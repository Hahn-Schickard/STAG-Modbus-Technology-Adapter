#include "Burst.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>

namespace Technology_Adapter::Modbus {

namespace Implementation {

// A mutable version of `BurstPlan`.
// We need mutability only in the constructor.
struct MutableBurstPlan {

  std::vector<BurstPlan::Burst> bursts;
  std::size_t num_plan_registers = 0;
  std::vector<std::size_t> task_to_plan;

  MutableBurstPlan(
      BurstPlan::Task const& task,
      RegisterSet const& holding,
      RegisterSet const& input,
      std::size_t max_burst_size)
      : task_to_plan(task.size()) {

    // If the task is empty, there is nothing left to do
    if (task.empty()) {
      return;
    }

    /*
      We use dynamic programming.
      For each register `r` from `task`, we compute the optimum plan for the
      subtask starting with `r`.
    */

    using ReverseTask = std::map<RegisterIndex, std::set<std::size_t>>;

    ReverseTask reverse_task;
    for (std::size_t i = 0; i < task.size(); ++i) {
      reverse_task.try_emplace(task[i]).first->second.insert(i);
    }
    // Now, `reverse_task[r]` holds all `i` such that `t[i] == r`.

    // We use iterators over `reverse_task` to iterate over registers.
    // Most of the time we do not care about the `->second` part of those.
    auto const& used_registers = reverse_task;
    using Forward = ReverseTask::const_iterator;
    using Backward = ReverseTask::const_reverse_iterator;

    struct Cost {
      size_t length;
      size_t total_size;

      bool operator<(Cost const& other) const {
        return (length < other.length) ||
            ((length == other.length) && (total_size < other.total_size));
      }
    };

    /*
      The "plan" computed for a register `r` is not actually a `BurstPlan` or
      `MutableBurstPlan`. That would make the operations performed in the
      computation rather expensive. Instead, it is a linked list of
      `RegisterRange`s, where different lists may share nodes.
    */
    struct Node {
      RegisterRange range;
      LibModbus::ReadableRegisterType type;
      std::shared_ptr<Node> next;
      Node(
          RegisterIndex start, RegisterIndex end, 
          LibModbus::ReadableRegisterType type_, std::shared_ptr<Node>&& next_)
          : range(start, end), type(type_), next(std::move(next_)) {}
    };
    struct List {
      std::shared_ptr<Node> head;
      Cost cost;
      List(std::shared_ptr<Node> const& head_, Cost const& cost_)
          : head(head_), cost(cost_) {}
    };
    std::map<RegisterIndex, List> optima;

    auto cost = [&optima, &used_registers](
        Forward const& next, size_t front_size) -> Cost {

      Cost cost = {1, front_size};
      if (next != used_registers.cend()) {
        auto const& optimum_for_next = optima.at(next->first);
        cost.length += optimum_for_next.cost.length;
        cost.total_size += optimum_for_next.cost.total_size;
      }
      return cost;
    };

    for ( // NOLINTNEXTLINE(modernize-use-auto)
        Backward i = used_registers.crbegin(); i != used_registers.crend();
        ++i) {
      // Invariant: `optima` has been populated for all registers after `r`.
      RegisterIndex r = i->first;

      /*
        We will loop through all potential bursts starting at `r`.
        For each candidate we compute the cost.
      */

      LibModbus::ReadableRegisterType type;
      RegisterIndex limit = (RegisterIndex)(r + max_burst_size - 1);
      if (holding.contains(r)) {
        if (input.contains(r)) {
          auto holding_range = holding.endOfRange(r);
          auto input_range = input.endOfRange(r);
          if (holding_range >= input_range) {
            type = LibModbus::ReadableRegisterType::HoldingRegister;
            limit = std::min(limit, holding_range);
          } else {
            type = LibModbus::ReadableRegisterType::InputRegister;
            limit = std::min(limit, input_range);
          }
        } else {
          type = LibModbus::ReadableRegisterType::HoldingRegister;
          limit = std::min(limit, holding.endOfRange(r));
        }
      } else {
        if (input.contains(r)) {
          type = LibModbus::ReadableRegisterType::InputRegister;
          limit = std::min(limit, input.endOfRange(r));
        } else {
          throw std::runtime_error("Unreadable register " + r);
        }
      }

      // NOLINTNEXTLINE(modernize-use-auto)
      Forward next = i.base(); // forward and reverse iterators differ by 1
      // NOLINTNEXTLINE(modernize-use-auto)
      Forward current = next;
      --current;
      // Now, `current` is a `Forward` version of `i`

      RegisterIndex best_burst_end = current->first;
      auto best_next = next;
      Cost best_cost = cost(best_next, 1);
      // Now, the loop has been initialized

      while ((next != used_registers.cend()) && (next->first <= limit)) {
        current = next;
        ++next;
        Cost current_cost = cost(next, current->first - r + 1);
        if (current_cost < best_cost) {
          best_burst_end = current->first;
          best_next = next;
          best_cost = current_cost;
        }
      }

      std::shared_ptr<Node> tail;
      if (best_next != used_registers.cend()) {
        tail = optima.at(best_next->first).head;
      }
      auto node =
          std::make_shared<Node>(r, best_burst_end, type, std::move(tail));
      optima.try_emplace(r, std::move(node), best_cost);
    }

    // Now, dynamic programming is finished. We collect the result.

    auto current = reverse_task.cbegin();
    auto const& list = optima.at(current->first);
    for (std::shared_ptr<Node> node = list.head; node; node = node->next) {
      std::size_t size = node->range.end - node->range.begin + 1;
      bursts.emplace_back(node->range.begin, node->type, size);
      for ( //
          ; (current != reverse_task.cend()) &&
          (current->first <= node->range.end);
          ++current) {

        std::size_t plan_number =
            num_plan_registers + current->first - node->range.begin;
        for (auto j : current->second) {
          task_to_plan[j] = plan_number;
        }
      }
      num_plan_registers += size;
    }
  }
};

} // namespace Implementation

BurstPlan::BurstPlan(Implementation::MutableBurstPlan&& source)
    : bursts(std::move(source.bursts)),
      num_plan_registers(source.num_plan_registers),
      task_to_plan(std::move(source.task_to_plan)) {}

BurstPlan::BurstPlan(
    Task const& task, RegisterSet const& holding, RegisterSet const& input,
    std::size_t max_burst_size)
    : BurstPlan(Implementation::MutableBurstPlan(
        task, holding, input, max_burst_size))
    {}

BurstBuffer::BurstBuffer(BurstPlan::Task const& task,
    RegisterSet const& holding, RegisterSet const& input,
    std::size_t max_burst_size)
    : plan(task, holding, input, max_burst_size),
      padded(plan.num_plan_registers), compact(task.size()) {}

} // namespace Technology_Adapter::Modbus
