#ifndef _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP

#include <list>
#include <map>

#include "Nonempty_Pointer/NonemptyPtr.hpp"

#include "Config.hpp"
#include "Index.hpp"

namespace Technology_Adapter::Modbus {

/*
  We are using `Threadsafe` so that instances of `Candidate` may be passed
  between threads.
*/
/**
 * @brief Covers the combinatorial part of port detection
 */
class PortFinderPlan : public Threadsafe::EnableSharedFromThis<PortFinderPlan> {
  class PortIndexingTag {};
  using PortIndexing = Indexing<Config::Portname, PortIndexingTag>;

  // used later to index buses per port, as opposed to globally
  class PortBusIndexingTag {};
  using PortBusIndexing =
      Indexing<Config::Bus::NonemptyPtr, PortBusIndexingTag>;

  struct SecretConstructorArgument {
    SecretConstructorArgument() = default;
  };

public:
  using NonemptyPtr =
      NonemptyPointer::NonemptyPtr<Threadsafe::SharedPtr<PortFinderPlan>>;

  class Candidate;

  using NewCandidates = std::vector<Candidate>;

  /// @brief A candidate for a bus/port assignment
  class Candidate {
  public:
    Candidate() = delete;

    /// @brief Only for internal use, yet public for technical reasons
    Candidate(SecretConstructorArgument, //
        NonemptyPtr plan, PortIndexing::Index port, PortBusIndexing::Index bus)
        : plan_(plan), port_(std::move(port)), bus_(std::move(bus)){};

    Config::Bus::NonemptyPtr const& getBus() const;
    Config::Portname const& getPort() const;

    /// This check should be performed before trying the candidate
    bool stillFeasible() const;

    /// To be called after successfully trying the candidate
    NewCandidates confirm() const;

  private:
    NonemptyPtr plan_;
    PortIndexing::Index port_;
    PortBusIndexing::Index bus_;
  };

  PortFinderPlan() = delete;

  /*
    We would like to have the constructor private. Yet then, we could not do
    `std::make_shared`. Hence it is public but not usable except in private
    contexts.
  */
  /// @brief Only for internal use, yet public for technical reasons
  PortFinderPlan(SecretConstructorArgument);

  /**
   * @brief Adds new buses to the plan
   *
   * @pre All entries of `new_buses` are in fact new to the plan
   */
  NewCandidates addBuses(
      std::vector<Config::Bus::NonemptyPtr> const& /*new_buses*/);

  NewCandidates unassign(Config::Portname const&);

  /*
    In the public interface, we provide a factory but no constructor. This is
    because the class has shared-from-this. All instances should be smartly
    heap-allocated. The factory function returning a smart pointer ensures that.
  */
  /// @brief Factory
  static NonemptyPtr make();

private:
  struct GlobalData;
  struct Port;

  using GlobalDataPtr =
      NonemptyPointer::NonemptyPtr<std::shared_ptr<GlobalData>>;

  mutable std::mutex mutex_; // protects the entire state
  GlobalDataPtr global_data_;

  /*
    `std::optional` for technical reasons: We need a default constructor

    Actually, we have an invariant:
    - holds a non-empty value for all of `global_data_->port_indexing`
  */
  IndexMap<Config::Portname, std::optional<Port>, PortIndexingTag> ports_;

  bool feasible(PortBusIndexing::Index, PortIndexing::Index) const;
  Port const& getPort(PortIndexing::Index) const;
  Port& getPort(PortIndexing::Index);

  /*
    Adds a Candidate, under the condition that it is feasible.

    To be used in situations where the created candidate is, in fact, newly
    possible.
  */
  void considerCandidate(
      NewCandidates&, PortBusIndexing::Index, PortIndexing::Index);

  NewCandidates assign(PortBusIndexing::Index, PortIndexing::Index);
};

} // namespace Technology_Adapter::Modbus

#include "implementations/PortFinderPlan__implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP
