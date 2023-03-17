#include "Burst.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace Modbus_Technology_Adapter {

namespace Implementation {

struct BurstMaker { // Prepares one burst

  BurstMaker() = delete;

  // Lifetime of `readable` must include lifetime of `this`
  BurstMaker(RegisterSet const& readable, std::size_t max_burst_size)
      : readable_(readable), max_burst_size_(max_burst_size) {}

  void startBurst(int start_register) {
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
  std::size_t num_plan_registers;
  std::vector<std::size_t> task_to_plan;

  MutableBurstPlan(BurstPlan::Task const& task, RegisterSet const& readable,
      std::size_t max_burst_size)
      : task_to_plan(task.size()) {

    std::map<int, std::set<std::size_t>> reverse_task;
    for (std::size_t i = 0; i < task.size(); ++i)
      reverse_task.try_emplace(task[i]).first->second.insert(i);
    // Now, `reverse_task[r]` holds all `i` such that `t[i] == r`.

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
