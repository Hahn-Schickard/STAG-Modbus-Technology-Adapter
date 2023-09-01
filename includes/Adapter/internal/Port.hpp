#ifndef _MODBUS_TECHNOLOGY_ADAPTER_PORT_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_PORT_HPP

#include <functional>
#include <optional>
#include <thread>

#include <HaSLL/Logger.hpp>
#include <Threadsafe_Containers/List.hpp>
#include <Threadsafe_Containers/Resource.hpp>

#include "internal/PortFinderPlan.hpp"

///  @brief Port detection from the point of view of a single port

namespace Technology_Adapter::Modbus {

class Port {
public:
  using SuccessCallback = std::function<void(PortFinderPlan::Candidate const&)>;

  Port() = delete;

  /**
   * @brief constructor
   *
   * Guarantee: The `SuccessCallback` is only called from the search thread.
   */
  Port(Config::Portname, SuccessCallback);

  /**
   * @brief Adds a candidate
   *
   * May block.
   *
   * @pre `candidate.getPort() == port_`
   */
  void addCandidate(PortFinderPlan::Candidate const& candidate);

  /// @brief Terminates the search thread, if any
  void stop();

private:
  enum struct TryResult {
    NoPort,
    NotFound,
    Found,
  };

  void search();

  TryResult tryCandidate(PortFinderPlan::Candidate const&) noexcept;

  // return value is success
  // Precondition: `context` is connected
  bool tryCandidate(
      PortFinderPlan::Candidate const&,
      LibModbus::ContextRTU& context) noexcept;

  enum struct State {
    Idle, // we would be searching, but lack candidates
    WakingUp, // we intend to start searching, but are not yet quite ready
    Searching,
    Found,
    Stopping, // `stop` has been called, no new search allowed
  };

  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> const logger_;
  Config::Portname const port_;
  SuccessCallback const success_callback_;
  Threadsafe::Resource<State> state_ = State::Idle;
  Threadsafe::Resource<std::optional<std::thread>> search_thread_;
  Threadsafe::List<PortFinderPlan::Candidate> candidates_;

  /*
    Invariants:
    - if `state_` is `Searching`, then `search_thread_` is non-empty
    - at most one thread may run `this->search` at a time
    - only the following `state_` transitions are possible:
      - `Idle` -> `WakingUp` -> `Searching` -> `Idle`
      - any of the above -> `Found`
      - any of the above -> `Stopping`
  */
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_HPP
