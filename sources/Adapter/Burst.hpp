#ifndef _MODBUS_TECHNOLOGY_ADAPTER_BURST_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_BURST_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "internal/LibmodbusAbstraction.hpp"
#include "internal/RegisterSet.hpp"

/**
 * @brief Manage burst combinatorics
 *
 * A burst consists of several consecutive registers read at once.
 * On the semantic side, grouping of registers is not necessarily consecutive.
 * Furthermore, the burst size of a device may be limited.
 * The present module handles the transition from semantic register groups
 * to bursts.
 */

namespace Technology_Adapter::Modbus {

namespace Implementation {
struct MutableBurstPlan;
}

/**
 * @brief Provides an optimized set of `Burst`s for a given `Task`.
 *
 * We distinguish between device register numbers and plan register numbers.
 *
 * The primary objective for optimization is number of bursts.
 * The secondary objective is total size of bursts.
 * The actual optimization computation happens in the constructor.
 */
struct BurstPlan {
  using Task = std::vector<RegisterIndex>; /// device register numbers

  /**
   * With each `Burst`, we associate plan registers.
   */
  struct Burst {
    RegisterIndex const start_register; /// first device register number

    LibModbus::ReadableRegisterType type;

    /**
     * Both, the number of consecutive device registers to read,
     * and the number of consecutive plan registers for this `Burst`.
     */
    int const num_registers;

    Burst() = delete;
    Burst(RegisterIndex start_register_, LibModbus::ReadableRegisterType type_,
        int num_registers_)
        : start_register(start_register_), type(type_),
          num_registers(num_registers_) {}
  };

  /**
   * The first plan register of each `Burst` is the sum over all previous
   * `Bursts`s of `num_registers`.
   */
  std::vector<Burst> const bursts;

  /// the sum over `bursts` of `num_registers`
  std::size_t const num_plan_registers;

  /**
   * For some `Task t` and all `0 <= i < p.size()`,
   * `task_to_plan[i]` is the plan register number corresponding to `t[i]`.
   */
  std::vector<std::size_t> const task_to_plan;

  BurstPlan() = delete;

  /// @throws `std::runtime_error` if `t` is impossible
  BurstPlan( //
      Task const& t /** as in the documentation for `task_to_plan` */,
      RegisterSet const& readable_holding_registers,
      RegisterSet const& readable_input_registers, //
      std::size_t max_burst_size);

private:
  BurstPlan(Implementation::MutableBurstPlan&&);
};

/**
 * @brief Bundles a `BurstPlan` with the buffers needed for operation.
 */
struct BurstBuffer {
  BurstPlan const plan;
  std::vector<uint16_t> padded; /// of size `plan.num_plan_registers`

  /// of size `task.size()`, where `task` refers to the constructor argument
  std::vector<uint16_t> compact;

  /// @throws `std::runtime_error` if `task` is impossible
  BurstBuffer(BurstPlan::Task const& task,
      RegisterSet const& readable_holding_registers,
      RegisterSet const& readable_input_registers, //
      std::size_t max_burst_size);
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_BURST_HPP
