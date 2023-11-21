
#include <optional>

#include "Index/MemoizedFunction.hpp"
#include "Index/Set.hpp"

namespace Technology_Adapter::Modbus {

namespace Internal_ {

class GlobalBusIndexingTag {};

using GlobalBusIndexingParameters =
    std::tuple<Config::Bus::NonemptyPtr, Internal_::GlobalBusIndexingTag>;

using GlobalBusIndexing =
    Index::Indexing<Config::Bus::NonemptyPtr, GlobalBusIndexingTag>;

template <class T>
using GlobalBusMap =
    Index::Map<Config::Bus::NonemptyPtr, T, GlobalBusIndexingTag>;

} // namespace Internal_

// The data that `PortFinderPlan` stores per port
struct PortFinderPlan::Port {
  using PortBusSet = Index::Set<Config::Bus::NonemptyPtr, PortBusIndexingTag>;

  template <class T>
  using PortBusMap =
      Index::Map<Config::Bus::NonemptyPtr, T, PortBusIndexingTag>;

  GlobalDataPtr global_data;
  PortBusIndexing bus_indexing;

  /*
    `std::optional` for technical reasons: We need a default constructor

    Actually, we have an invariant:
    - holds a non-empty value for all of `bus_indexing`
  */
  PortBusMap<std::optional<Internal_::GlobalBusIndexing::Index>>
      global_bus_index;

  // maps local buses to other local buses which they ambiguate
  PortBusMap<std::vector<PortFinderPlan::PortBusIndexing::Index>> ambiguated;

  // The bus (if any) that is currently assigned to the port
  std::optional<PortFinderPlan::PortBusIndexing::Index> assigned;

  /*
    Unassigned `Bus`es which list this port as a possibility.

    If `assigned.has_value()`, then this represents the state after the
    respective `unassign()`. Otherwise it represents the current state.
  */
  PortBusSet available;

  /*
    Per `Bus`, number of available other `Bus`es that ambiguate it.

    If `assigned.has_value()`, then this represents the state after the
    respective `unassign()`. Otherwise it represents the current state.
  */
  PortBusMap<size_t> num_ambiguators; // counts only available ones

  Port(GlobalDataPtr const&);

  Internal_::GlobalBusIndexing::Index globalBusIndex(
      PortBusIndexing::Index) const;

  // Returns the local index of the new bus
  PortFinderPlan::PortBusIndexing::Index addBus(
      Internal_::GlobalBusIndexing::Index);
};

// The data that `PortFinderPlan` stores just once
struct PortFinderPlan::GlobalData {
  using BusIndexing = NonemptyPointer::NonemptyPtr<
      std::shared_ptr<Internal_::GlobalBusIndexing>>;
  using BinaryBusPredicate = Index::MemoizedFunction<bool,
      Internal_::GlobalBusIndexingParameters,
      Internal_::GlobalBusIndexingParameters>;
  using Incidence = std::pair< // describes a potential assignment port->bus
      PortFinderPlan::PortIndexing::Index,
      PortFinderPlan::PortBusIndexing::Index>;

  BusIndexing bus_indexing;
  PortIndexing port_indexing;
  BinaryBusPredicate const ambiguates;

  // maps `Bus`es (global index) to `Incidence`s (local index) of that `Bus`
  Internal_::GlobalBusMap<std::vector<Incidence>> possible_ports;

  GlobalData();
};

} // namespace Technology_Adapter::Modbus
