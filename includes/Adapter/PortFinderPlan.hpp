#ifndef _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP

#include <list>
#include <map>

#include "Nonempty_Pointer/NonemptyPtr.hpp"

#include "Config.hpp"
#include "Index.hpp"

namespace Technology_Adapter::Modbus {

/**
 * @brief Covers the combinatorial part of port detection
 */
class PortFinderPlan : public Threadsafe::EnableSharedFromThis<PortFinderPlan> {
  class PortIndexingTag {};
  using PortIndexing = Indexing<Config::Portname, PortIndexingTag>;

  class PortBusIndexingTag {};
  using PortBusIndexing = Indexing<Config::Bus::Ptr, PortBusIndexingTag>;

public:
  using Ptr = Threadsafe::SharedPtr<PortFinderPlan>;
  using NonemptyPtr = NonemptyPointer::NonemptyPtr<Ptr>;

  class Candidate;

  using NewCandidates = std::vector<Candidate>;

  class Candidate {
  public:
    Config::Bus::Ptr const& getBus() const;
    Config::Portname const& getPort() const;

    /// This check should be performed before trying the candidate
    bool stillFeasible() const;

    /// To be called after successfully trying the candidate
    NewCandidates confirm();

  private:
    NonemptyPtr plan_;
    PortIndexing::Index port_;
    PortBusIndexing::Index bus_;

    Candidate() = delete;
    Candidate(
        NonemptyPtr plan, PortIndexing::Index port, PortBusIndexing::Index bus)
        : plan_(plan), port_(std::move(port)), bus_(std::move(bus)) {};

    friend class PortFinderPlan;
  };

  PortFinderPlan();

  NewCandidates addBuses(std::vector<Config::Bus::Ptr> const&);
  NewCandidates unassign(Config::Portname const&);

private:
  struct NonPortData;
  struct Port;

  using NonPortDataPtr =
      NonemptyPointer::NonemptyPtr<std::shared_ptr<NonPortData>>;

  NonPortDataPtr non_port_data_;
  IndexMap<Config::Portname, std::optional<Port>, PortIndexingTag> ports_;

  bool feasible(PortBusIndexing::Index, PortIndexing::Index) const;

  NewCandidates assign(PortBusIndexing::Index, PortIndexing::Index);
};

} // namespace Technology_Adapter::Modbus

#include "implementations/PortFinderPlan__implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP
