
#include <optional>

namespace Technology_Adapter::Modbus {

namespace Internal_ {

class GlobalBusIndexingTag {};

using GlobalBusIndexing = Indexing<Config::Bus::Ptr, GlobalBusIndexingTag>;

template <class T>
using GlobalBusMap = IndexMap<Config::Bus::Ptr, T, GlobalBusIndexingTag>;

} // namespace Internal_

struct PortFinderPlan::Port {
  using PortBusSet = IndexSet<Config::Bus::Ptr, PortBusIndexingTag>;

  template <class T>
  using PortBusMap = IndexMap<Config::Bus::Ptr, T, PortBusIndexingTag>;

  NonPortDataPtr non_port_data;
  PortBusIndexing bus_indexing;

  // `std::optional` for technical reasons: We need a default constructor
  PortBusMap<std::optional<Internal_::GlobalBusIndexing::Index>>
      global_bus_index;

  PortBusMap<std::vector<PortFinderPlan::PortBusIndexing::Index>> ambiguated;

  std::optional<Config::Bus::Ptr> assigned;

  /*
    If `assigned.has_value()`, then the values of the following represent the
    state after the respective `unassign()`.
  */
  PortBusSet available;
  PortBusMap<size_t> num_ambiguators; // counts only available ones

  Port(NonPortDataPtr const&);

  bool isBusUnique(Config::Bus::Ptr const&) const;

  PortFinderPlan::PortBusIndexing::Index addBus(Config::Bus::Ptr const&);
  void makeBusAvailable(PortFinderPlan::PortBusIndexing::Index const&);
};

struct PortFinderPlan::NonPortData {
  using BusIndexing =
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Internal_::GlobalBusIndexing>>;
  using BinaryBusPredicate = MemoizedBinaryFunction<
      Config::Bus::Ptr, Config::Bus::Ptr, bool, Internal_::GlobalBusIndexingTag,
      Internal_::GlobalBusIndexingTag>;
  using BusSet = IndexSet<Config::Bus::Ptr, Internal_::GlobalBusIndexingTag>;

  BusIndexing bus_indexing;
  PortIndexing port_indexing;
  BinaryBusPredicate ambiguates;
  Internal_::GlobalBusMap<std::vector<std::pair<PortFinderPlan::PortIndexing::Index, PortFinderPlan::PortBusIndexing::Index>>>
      possible_ports;
  BusSet assigned;

  NonPortData();
};

} // namespace Technology_Adapter::Modbus
