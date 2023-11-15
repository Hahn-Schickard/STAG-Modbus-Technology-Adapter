#include <HaSLL/LoggerManager.hpp>

#include "internal/Port.hpp"

namespace Technology_Adapter::Modbus {

/*
  This is how long a we wait between attempts in case there is no port.
*/
constexpr size_t HOTPLUG_WAIT_TIME_MS = 100;

Port::Port(LibModbus::Context::Factory context_factory, Config::Portname port,
    SuccessCallback success_callback)
    : context_factory_(std::move(context_factory)),
      logger_(HaSLI::LoggerManager::registerLogger(
          std::string((std::string_view)("Modbus Adapter port " + port)))),
      port_(std::move(port)), success_callback_(std::move(success_callback)) {

  logger_->trace("state is Idle");
}

void Port::addCandidate(PortFinderPlan::Candidate const& candidate) {
  logger_->debug("Adding candidate {}", candidate.getBus()->id);
  bool wake_up;
  {
    auto state_access = state_.lock();
    switch (*state_access) {
    case State::Idle:
      candidates_.clear();
      candidates_.emplace_front(candidate);
      wake_up = true;
      *state_access = State::WakingUp;
      logger_->trace("state is WakingUp");
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
      `WakingUp` -> `Searching` -> `Idle` transitions. Yet the first of these
      transitions concludes this scope.
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
      logger_->trace("state is Searching");
      break;
    default:
      throw std::logic_error("Internal error");
    }
  }
}

void Port::reset() {
  stop_thread();

  *state_.lock() = State::Idle;
  logger_->trace("state is Idle");
}

void Port::stop() {
  logger_->trace("Stopping");

  stop_thread();
}

void Port::stop_thread() {
  *state_.lock() = State::Stopping;
  logger_->trace("state is Stopping");

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

  bool no_port = true; // result was `NoPort` for all candidates

  while ((*state_.lock() == State::Searching) &&
      (next_candidate != candidates_.end())) {

    auto candidate = next_candidate;
    ++next_candidate;

    if (candidate->stillFeasible()) {
      switch (tryCandidate(*candidate)) {
      case TryResult::NoPort:
        break;
      case TryResult::NotFound:
        no_port = false;
        break;
      case TryResult::Found: {
        bool was_still_searching;
        {
          auto state_access = state_.lock();
          was_still_searching = *state_access == State::Searching;
          if (was_still_searching) {
            *state_access = State::Found;
            logger_->trace("state is Found");
          }
        }
        if (was_still_searching) {
          success_callback_(*candidate);
        }
      } break;
      default:
        throw std::logic_error("Incomplete switch");
      }
    } else {
      logger_->debug("{} no longer feasible", candidate->getBus()->id);
      candidates_.erase(candidate);
    }

    if ((*state_.lock() == State::Searching) &&
        (next_candidate == candidates_.end())) {

      if (no_port) {
        /*
          The next round of attempts will fail just the same unless some
          hardware is hot-plugged. We may just as well wait a bit.
        */
        std::this_thread::sleep_for(
            std::chrono::milliseconds(HOTPLUG_WAIT_TIME_MS));
      }

      next_candidate = candidates_.begin();
      no_port = true;
    }
  }
  logger_->trace("Finishing search");
}

Port::TryResult Port::tryCandidate(
    PortFinderPlan::Candidate const& candidate) noexcept {

  logger_->debug("Trying {}", candidate.getBus()->id);
  try {
    auto const& bus = *candidate.getBus();
    auto context = context_factory_(port_, bus);
    try {
      context->connect();
      bool result = tryCandidate(candidate, context);
      context->close();
      return result ? TryResult::Found : TryResult::NotFound;
    } catch (std::exception const& exception) {
      /*
        Above, everything after `connect` is `noexcept.
        Hence it was `connect` that threw.
        Thus we don't have to `close` the context.
      */
      logger_->error("While connecting: {}", exception.what());
      return TryResult::NoPort;
    }
  } catch (std::exception const& exception) {
    logger_->error("While creating context: {}", exception.what());
    return TryResult::NoPort;
  }
}

bool Port::tryCandidate(PortFinderPlan::Candidate const& candidate,
    LibModbus::Context::Ptr const& context) noexcept {

  auto const& bus = *candidate.getBus();
  try {
    uint16_t value;
    for (auto const& device : bus.devices) {
      context->selectDevice(*device);
      for (auto holding_register : device->holding_registers) {
        logger_->trace("Trying to read holding register {} of {}",
            holding_register, device->id);
        try {
          int num_read = context->readRegisters(holding_register,
              LibModbus::ReadableRegisterType::HoldingRegister, 1, &value);
          if (num_read != 1) {
            logger_->debug("Holding register {} of {} could not be read",
                holding_register, device->id);
            return false;
          }
        } catch (std::exception const& exception) {
          logger_->error("Holding register {} of {} could not be read: {}",
              holding_register, device->id, exception.what());
          return false;
        }
      }
      for (auto input_register : device->input_registers) {
        logger_->trace("Trying to read input register {} of {}", input_register,
            device->id);
        try {
          int num_read = context->readRegisters(input_register,
              LibModbus::ReadableRegisterType::InputRegister, 1, &value);
          if (num_read != 1) {
            logger_->debug("Input register {} of {} could not be read",
                input_register, device->id);
            return false;
          }
        } catch (std::exception const& exception) {
          logger_->error("Input register {} of {} could not be read: {}",
              input_register, device->id, exception.what());
          return false;
        }
      }
    }

    // If control reaches this point, all registers could be read
    logger_->debug("{} was successful", candidate.getBus()->id);
    return true;
  } catch (std::exception const& exception) {
    logger_->error("While trying candidate {}: {}}", bus.id, exception.what());
    return false;
  }
}

} // namespace Technology_Adapter::Modbus
