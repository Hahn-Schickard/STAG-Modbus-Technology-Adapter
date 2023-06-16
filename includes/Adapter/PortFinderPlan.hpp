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
    Config::Bus::Ptr bus_;
    Config::Portname port_;
    NonemptyPtr plan_;

    Candidate() = delete;
    Candidate(Config::Bus::Ptr bus, Config::Portname port, NonemptyPtr plan)
        : bus_(std::move(bus)), port_(std::move(port)), plan_(plan){};

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
  std::map<std::string, Port> ports_by_name_;

  bool feasible(Config::Bus::Ptr const&, Config::Portname const&) const;
  NewCandidates assign(Config::Bus::Ptr const&, Config::Portname const&);
};

} // namespace Technology_Adapter::Modbus

#include "implementations/PortFinderPlan__implementation.hpp"

#endif // _MODBUS_TECHNOLOGY_ADAPTER_PORT_FINDER_PLAN_HPP
