#include "Burst.hpp"

#include <map>
#include <set>

namespace Modbus_Technology_Adapter {

namespace Implementation {

struct BurstMaker { // Prepares one burst

  BurstMaker() = delete;
  BurstMaker(size_t max_burst_size) : max_burst_size_(max_burst_size) {}

  void startBurst(int start_register) {
    start_register_ = start_register;
    burst_size_ = 0;
    limit_ = start_register + max_burst_size_;
  }

  bool addRegister(int next_register) {
    // Return value indicates whether the register did fit

    bool fits = next_register < limit_;
    if (fits)
      burst_size_ = next_register - start_register_ + 1;
    return fits;
  }

  size_t planNumber(int current_register /* precondition: in current burst */) {
    return total_size_ + (current_register - start_register_);
  }

  BurstPlan::Burst finishBurst() {
    total_size_ += burst_size_;
    return BurstPlan::Burst(start_register_, burst_size_);
  }

  size_t totalSize() { return total_size_; }

private:
  size_t max_burst_size_;
  int start_register_;
  size_t burst_size_;
  int limit_;
  size_t total_size_ = 0;
};

// A mutable version of `BurstPlan`.
// We need mutability only in the constructor.
struct MutableBurstPlan {

  std::vector<BurstPlan::Burst> bursts;
  size_t num_plan_registers;
  std::vector<size_t> task_to_plan;

  MutableBurstPlan(BurstPlan::Task const& task, size_t max_burst_size)
      : task_to_plan(task.size()) {

    std::map<int,std::set<size_t>> reverse_task;
    for (size_t i = 0; i < task.size(); ++i)
      reverse_task.try_emplace(task[i]).first->second.insert(i);
    // Now, `reverse_task[r]` holds all `i` such that `t[i] == r`.

    BurstMaker maker(max_burst_size);
    auto i = reverse_task.cbegin();

    if (i != reverse_task.cend()) {
      maker.startBurst(i->first);

      while (i != reverse_task.cend()) {
        if (maker.addRegister(i->first)) {
          size_t plan_number = maker.planNumber(i->first);
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

BurstPlan::BurstPlan(Task const& task, size_t max_burst_size)
    : BurstPlan(Implementation::MutableBurstPlan(task, max_burst_size)) {}

BurstBuffer::BurstBuffer(BurstPlan::Task const& task, size_t max_burst_size)
    : plan(task, max_burst_size), padded(plan.num_plan_registers),
      compact(task.size()) {}

} // namespace Modbus_Technology_Adapter
