#include "Port.hpp"

namespace Technology_Adapter::Modbus {

Port::Port(Config::Portname port, SuccessCallback success_callback)
    : port_(std::move(port)), success_callback_(std::move(success_callback)) {}

void Port::addCandidate(PortFinderPlan::Candidate const& candidate) {
  bool wake_up;
  {
    auto state_access = state_.lock();
    switch (*state_access) {
    case State::Idle:
      candidates_.clear();
      candidates_.emplace_front(candidate);
      wake_up = true;
      *state_access = State::WakingUp;
      break;
      // start a new `search` thread
    case State::WakingUp: // another thread already wakes us up
    case State::Searching:
      candidates_.emplace_front(candidate);
      wake_up = false;
      break;
    case State::Found:
    case State::Stopping:
      wake_up = false;
      break;
    }
  }
  /*
    At this point, `state_access` is released.
    That is why we don't include the following in the `case State::Idle` above.
  */

  if (wake_up) {
    /*
      Only one thread may be in this scope at the same time. Namely the thread
      that set `state_` to `WakingUp`. This cannot happen again before the
      `WakingUp` -> `Searching` -> `Idle` transitions. Yet the former concludes
      this scope.
    */

    auto search_thread_access = search_thread_.lock();
    /*
      We may lock `search_thread_` for a long time. This is fine: Here, we are
      the only thread and the other code that wants to access `search_thread_`
      is in `stop`, which has to wait anyway.
    */

    if (search_thread_access->has_value()) {
      /*
        There is a thread, but it has been signalled by `Idle` to
        come to an end. We wait till it does and then start a new one.
      */
      // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
      search_thread_access->value().join();
    }

    auto state_access = state_.lock();
    switch (*state_access) {
    case State::Found:
    case State::Stopping:
      break;
    case State::WakingUp:
      search_thread_access->emplace([this] { this->search(); });
      *state_access = State::Searching;
      break;
    default:
      throw std::logic_error("Internal error");
    }
  }
}

void Port::stop() {
  *state_.lock() = State::Stopping;

  auto search_thread_access = search_thread_.lock();
  /*
    We may lock `search_thread_` for a long time. This may block `addCandidate`.
  */

  if (search_thread_access->has_value()) {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    search_thread_access->value().join();
    search_thread_access->reset();
  }
}

void Port::search() {
  auto next_candidate = candidates_.begin();

  while ((*state_.lock() == State::Searching) &&
      (next_candidate != candidates_.end())) {

    auto candidate = next_candidate;
    ++next_candidate;

    if (candidate->stillFeasible()) {
      if (tryCandidate(*candidate)) {
        *state_.lock() = State::Found;
        success_callback_(*candidate);
      }
    } else {
      candidates_.erase(candidate);
    }

    if ((*state_.lock() == State::Searching) &&
        (next_candidate == candidates_.end())) {

      next_candidate = candidates_.begin();
    }
  }
}

bool Port::tryCandidate(PortFinderPlan::Candidate const& candidate) {
  try {
    auto const& bus = *candidate.getBus();
    LibModbus::ContextRTU context(
        port_, bus.baud, bus.parity, bus.data_bits, bus.stop_bits);

    context.connect();
    try {
      uint16_t value;
      for (auto const& device : bus.devices) {
        context.setSlave(device.slave_id);
        for (auto holding_register : device.holding_registers) {
          int num_read = context.readRegisters(holding_register,
              LibModbus::ReadableRegisterType::HoldingRegister, 1, &value);
          if (num_read != 1) {
            // We do throw a value. clang-tidy complaining is probably a bug
            // NOLINTNEXTLINE(cert-err09-cpp,cert-err61-cpp)
            throw num_read;
          }
        }
        for (auto input_register : device.input_registers) {
          int num_read = context.readRegisters(input_register,
              LibModbus::ReadableRegisterType::InputRegister, 1, &value);
          if (num_read != 1) {
            // We do throw a value. clang-tidy complaining is probably a bug
            // NOLINTNEXTLINE(cert-err09-cpp,cert-err61-cpp)
            throw num_read;
          }
        }
      }
      context.close();

      // If control reaches this point, all registers could be read

      return true;
    } catch (...) {
      context.close();
      throw;
    }
  } catch (...) {
    return false;
  }
}

} // namespace Technology_Adapter::Modbus
