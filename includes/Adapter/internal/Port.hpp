#ifndef _MODBUS_TECHNOLOGY_ADAPTER_PORT_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_PORT_HPP

#include <functional>
#include <optional>
#include <thread>

#include <HaSLL/Logger.hpp>
#include <Threadsafe_Containers/List.hpp>
#include <Threadsafe_Containers/Resource.hpp>

#include "PortFinderPlan.hpp"

/// @brief Port detection from the point of view of a single port

namespace Technology_Adapter::Modbus {

/**
 * @briefs Can run a search thread for buses on a given port
 *
 * For analysis, we pretend there were a member `bool assigned`.
 * - The `!assigned` -> `assigned` transition happens internally and triggers
 *   the `SuccessCallback`.
 * - The `assigned` -> `!assigned` transition happens through `reset`.
 */
class Port {
public:
  using SuccessCallback = std::function<void(PortFinderPlan::Candidate const&)>;

  Port() = delete;

  /**
   * @brief constructor
   *
   * Guarantee: The `SuccessCallback` is only called from the search thread.
   *
   * @post `!assigned`
   */
  Port(LibModbus::Context::Factory, Config::Portname, SuccessCallback);

  /// Terminates the search thread, if any
  ~Port();

  /**
   * @brief Adds a candidate
   *
   * May block.
   *
   * @pre `candidate.getPort() == port_`
   * @pre No other call to `addCandidate` or `reset` is in process
   */
  void addCandidate(PortFinderPlan::Candidate const& candidate);

  /// @pre 'assigned`
  /// @pre No other call to `addCandidate` or `reset` is in process
  /// @post `!assigned`
  void reset();

private:
  // The result of looking for a given bus on the port
  enum struct TryResult {
    NoPort, // the port does not exist
    NotFound, // the bus was not found
    Found,
  };

  struct Search;

  void stopThread();
  void search(); // to be run in its own thread

  // @pre `candidate.getPort() == port_`
  TryResult tryCandidate(PortFinderPlan::Candidate const& candidate) noexcept;

  // return value is success
  // @pre `context` is connected
  bool tryCandidate( //
      PortFinderPlan::Candidate const&,
      LibModbus::Context::Ptr const& context) noexcept;

  enum struct State {
    Idle, // we would be searching, but lack candidates
    Searching,
    Found, // we have affirmed a candidate
    OutOfCandidates, // like `Idle`, but the old search thread may still run
    Stopping, // `stop` has been called, no new search allowed
  };

  LibModbus::Context::Factory const context_factory_;
  NonemptyPointer::NonemptyPtr<HaSLI::LoggerPtr> const logger_;
  Config::Portname const port_;
  SuccessCallback const success_callback_;
  Threadsafe::Resource<State> state_ = State::Idle;
  std::optional<std::thread> search_thread_;

  // The search cycles through this list
  // Meaningful only in state `Searching`
  Threadsafe::List<PortFinderPlan::Candidate> candidates_;

  /*
    Invariants:
    - Only `search_thread_` may run `this->search`
    - If `state_` is `Idle`, then `search_thread_` is empty
    - If `state_` is `Searching` or `OutOfCandidates`, then `search_thread_` is
      non-empty
    - Only the following `state_` transitions are possible:
      - `Idle` -> `Searching` -> `Found`
      - `Searching` -> `OutOfCandidates`
      - any of the above -> `Stopping` -> `Idle`
  */
};

} // namespace Technology_Adapter::Modbus

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_HPP
