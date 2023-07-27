
#include <optional>

namespace Technology_Adapter::Modbus {

namespace Internal_ {

class GlobalBusIndexingTag {};

using GlobalBusIndexing =
    Indexing<Config::Bus::NonemptyPtr, GlobalBusIndexingTag>;

template <class T>
using GlobalBusMap =
    IndexMap<Config::Bus::NonemptyPtr, T, GlobalBusIndexingTag>;

} // namespace Internal_

struct PortFinderPlan::Port {
  using PortBusSet = IndexSet<Config::Bus::NonemptyPtr, PortBusIndexingTag>;

  template <class T>
  using PortBusMap = IndexMap<Config::Bus::NonemptyPtr, T, PortBusIndexingTag>;

  GlobalDataPtr global_data;
  PortBusIndexing bus_indexing;

  // `std::optional` for technical reasons: We need a default constructor
  PortBusMap<std::optional<Internal_::GlobalBusIndexing::Index>>
      global_bus_index;

  // maps local buses to other local buses which they ambiguate
  PortBusMap<std::vector<PortFinderPlan::PortBusIndexing::Index>> ambiguated;
  std::optional<PortFinderPlan::PortBusIndexing::Index> assigned;

  /*
    If `assigned.has_value()`, then the values of the following represent the
    state after the respective `unassign()`.
  */
  PortBusSet available;
  PortBusMap<size_t> num_ambiguators; // counts only available ones

  Port(GlobalDataPtr const&);

  PortFinderPlan::PortBusIndexing::Index addBus(
      Internal_::GlobalBusIndexing::Index);
};

struct PortFinderPlan::GlobalData {
  using BusIndexing = NonemptyPointer::NonemptyPtr<
      std::shared_ptr<Internal_::GlobalBusIndexing>>;
  using BinaryBusPredicate = MemoizedBinaryFunction<
      Config::Bus::NonemptyPtr, Config::Bus::NonemptyPtr, bool,
      Internal_::GlobalBusIndexingTag, Internal_::GlobalBusIndexingTag>;
  using Incidence = std::pair< // describes a potential assignment port->bus
      PortFinderPlan::PortIndexing::Index,
      PortFinderPlan::PortBusIndexing::Index>;

  BusIndexing bus_indexing;
  PortIndexing port_indexing;
  BinaryBusPredicate ambiguates;
  Internal_::GlobalBusMap<std::vector<Incidence>> possible_ports;

  GlobalData();
};

} // namespace Technology_Adapter::Modbus
