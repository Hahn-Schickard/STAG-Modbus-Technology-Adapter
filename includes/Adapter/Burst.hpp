#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BURST_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BURST_HPP

#include <cstdint>
#include <vector>

#include "RegisterSet.hpp"

/**
 * @brief Manage burst combinatorics
 *
 * A burst consists of several consecutive registers read at once.
 * On the semantic side, grouping of registers is not necessarily consecutive.
 * Furthermore, the burst size of a device may be limited.
 * The present module handles the transition from semantic register groups
 * to bursts.
 */

namespace Modbus_Technology_Adapter {

namespace Implementation {
struct MutableBurstPlan;
}

/**
 * We distinguish between device register numbers and plan register numbers.
 */
struct BurstPlan {
  using Task = std::vector<int>; /// device register numbers

  /**
   * With each `Burst`, we associate plan registers.
   */
  struct Burst {
    int const start_register; /// first device register number

    /**
     * Both, the number of consecutive device registers to read,
     * and the number of consecutive plan registers for this `Burst`.
     */
    int const num_registers;

    Burst(int start_register_, int num_registers_)
        : start_register(start_register_), num_registers(num_registers_) {}
  };

  /**
   * The first plan register of each `Burst` is the sum over all previous
   * `Bursts`s of `num_registers`.
   */
  std::vector<Burst> const bursts;
  size_t const num_plan_registers; /// the sum over `bursts` of `num_registers`

  /**
   * For some `Task t` and all `0 <= i < p.size()`,
   * `task_to_plan[i]` is the plan register number corresponding to `t[i]`.
   */
  std::vector<size_t> const task_to_plan;

  BurstPlan() = delete;
  BurstPlan( //
    Task const& /** `t` as in the documentation for `task_to_plan` */,
    RegisterSet const& /*readable*/, size_t /*max_burst_size*/);

private:
  BurstPlan(Implementation::MutableBurstPlan&&);
};

/**
 * There is an implicit member `task`, which is the `BurstPlan::Task` passed to
 * the constructor of `plan`.
 */
struct BurstBuffer {
  BurstPlan plan;
  std::vector<uint16_t> padded; /// of size `plan.num_plan_registers`
  std::vector<uint16_t> compact; /// of size `task.size()`

  BurstBuffer(BurstPlan::Task const& /*task*/, RegisterSet const& /*readable*/,
      size_t /*max_burst_size*/);
};

} // namespace Modbus_Technology_Adapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BURST_HPP
