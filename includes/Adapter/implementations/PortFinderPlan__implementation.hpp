
#include <optional>

#include "Index.hpp"

namespace Technology_Adapter::Modbus {

namespace Internal_ {

class GlobalBusIndexingTag {};
class PortBusIndexingTag {};

using GlobalBusIndexing = Indexing<Config::Bus::Ptr, GlobalBusIndexingTag>;
using PortBusIndexing = Indexing<Config::Bus::Ptr, PortBusIndexingTag>;
using PortBusSet = IndexSet<Config::Bus::Ptr, PortBusIndexingTag>;

template <class T>
using GlobalBusMap = IndexMap<Config::Bus::Ptr, T, GlobalBusIndexingTag>;

template <class T>
using PortBusMap = IndexMap<Config::Bus::Ptr, T, PortBusIndexingTag>;

} // namespace Internal_

struct PortFinderPlan::NonPortData {
  using BusIndexing =
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Internal_::GlobalBusIndexing>>;
  using BinaryBusPredicate = MemoizedBinaryFunction<
      Config::Bus::Ptr, Config::Bus::Ptr, bool, Internal_::GlobalBusIndexingTag,
      Internal_::GlobalBusIndexingTag>;

  BusIndexing bus_indexing;
  BinaryBusPredicate contained_in;
  Internal_::GlobalBusMap<std::vector<std::pair<Config::Portname, Internal_::PortBusIndexing::Index>>>
      possible_ports;

  NonPortData();
};

struct PortFinderPlan::Port {
  NonPortDataPtr non_port_data;
  Internal_::PortBusIndexing bus_indexing;
  Internal_::PortBusMap<std::optional<Internal_::GlobalBusIndexing::Index>>
      global_bus_index;
  Internal_::PortBusMap<std::vector<Internal_::PortBusIndexing::Index>> smaller;
  Internal_::PortBusMap<std::vector<Internal_::PortBusIndexing::Index>> larger;

  std::optional<Config::Bus::Ptr> assigned;

  /*
    If `assigned.has_value()`, then the values of the following represent the
    state after the respective `unassign()`.
  */
  Internal_::PortBusSet available;
  Internal_::PortBusMap<size_t> num_available_larger;

  std::list<Config::Bus::Ptr> possible_buses;
      // subset of `all_buses`; "impossibility" due to contradicting assignment
  std::list<Config::Bus::Ptr> ambiguous_buses; // subset of `possible_buses`

  Port(NonPortDataPtr const&);

  bool isBusUnique(Config::Bus::Ptr const&) const;

  Internal_::PortBusIndexing::Index addBus(Config::Bus::Ptr const&);
  void makeBusAvailable(Internal_::PortBusIndexing::Index const&);
};

} // namespace Technology_Adapter::Modbus
