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

Port::~Port() {
  logger_->trace("Destructing");

  stopThread();
}

void Port::addCandidate(PortFinderPlan::Candidate const& candidate) {
  logger_->debug("Adding candidate {}", candidate.getBus()->id);
  {
    auto state_access = state_.lock();
    switch (*state_access) {
    case State::OutOfCandidates:
      // `search_thread` is non-empty by the invariant. Hence:
      // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
      search_thread_.value().join();

      // we fall through to the `Idle` case
    case State::Idle:
      // This is the first candidate. We need to start a thread.

      // By the precondition, neither `reset` nor another `addCandidate` runs.
      // `search` also does not run:
      // - If we entered via `OutOfCandidates`, we have already called `join`
      // - If we entered via `Idle`, `search_thread` is empty by the invariant.
      // The fact that nothing else runs provides the thread-safety we need.

      // previous candidates, if any, are left-over garbage
      candidates_.clear();
      candidates_.emplace_front(candidate);

      // start a new `search` thread
      *state_access = State::Searching;
      logger_->trace("state is Searching");
      search_thread_.emplace([this] { this->search(); });

      break;

    case State::Searching:
      candidates_.emplace_front(candidate);
      break;

    case State::Found:
    case State::Stopping:
      break;
    }
  }
}

void Port::reset() {
  stopThread();

  *state_.lock() = State::Idle;
  logger_->trace("state is Idle");
}

void Port::stopThread() {
  *state_.lock() = State::Stopping;
  logger_->trace("state is Stopping");

  if (search_thread_.has_value()) {
    search_thread_.value().join();
    search_thread_.reset();
  }
}

/*
  @brief Internal data for `Port::search()`
*/
struct Port::Search {
  Port& port;
  Threadsafe::List<PortFinderPlan::Candidate>::iterator next_candidate;
  bool some_port_existed;

  void tryCandidate() {
    if (next_candidate->stillFeasible()) {
      switch (port.tryCandidate(*next_candidate)) {
      case TryResult::NoPort:
        break;
      case TryResult::NotFound:
        some_port_existed = true;
        break;
      case TryResult::Found: {
        bool was_still_searching;
        {
          auto state_access = port.state_.lock();
          was_still_searching = *state_access == State::Searching;
          if (was_still_searching) {
            *state_access = State::Found;
            port.logger_->trace("state is Found");
          }
        }
        if (was_still_searching) {
          port.success_callback_(*next_candidate);
        }
      } break;
      default:
        throw std::logic_error("Incomplete switch");
      }
    } else {
      port.logger_->debug(
          "{} no longer feasible", next_candidate->getBus()->id);
      port.candidates_.erase(next_candidate);
    }
  }

  void next() {
    ++next_candidate;
    if ((*port.state_.lock() == Port::State::Searching) &&
        (next_candidate == port.candidates_.end())) {

      next_candidate = port.candidates_.begin();
      if (next_candidate == port.candidates_.end()) {
        *port.state_.lock() = Port::State::OutOfCandidates;
        port.logger_->trace("state is OutOfCandidates");
      } else {
        if (!some_port_existed) {
          /*
            The next round of attempts will fail just the same unless some
            hardware is hot-plugged. We may just as well wait a bit.
          */
          std::this_thread::sleep_for(
              std::chrono::milliseconds(HOTPLUG_WAIT_TIME_MS));
        }

        some_port_existed = false;
      }
    }
  }
};

void Port::search() {
  Search search{*this, candidates_.begin(), false};

  while (*state_.lock() == State::Searching) {
    if (search.next_candidate == candidates_.end()) {
      *state_.lock() = State::OutOfCandidates;
      logger_->trace("state is OutOfCandidates");
    } else {
      search.tryCandidate();
      search.next();
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
