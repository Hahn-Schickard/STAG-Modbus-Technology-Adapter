
#include <optional>

#include "Index.hpp"

namespace Technology_Adapter::Modbus {

namespace Internal_ {

class GlobalBusIndexing : public Indexing<Config::Bus::Ptr> {};

} // namespace Internal_

struct PortFinderPlan::NonPortData {
  using BusIndexing =
      NonemptyPointer::NonemptyPtr<std::shared_ptr<Internal_::GlobalBusIndexing>>;
  using BinaryBusPredicate =
      MemoizedBinaryFunction<Config::Bus::Ptr, Config::Bus::Ptr, bool>;

  BusIndexing bus_indexing;
  BinaryBusPredicate distinguishable_from;

  NonPortData();
};

struct PortFinderPlan::Port {
  NonPortDataPtr non_port_data;

  std::optional<Config::Bus::Ptr> assigned;
  std::list<Config::Bus::Ptr> all_buses;
  std::list<Config::Bus::Ptr> possible_buses;
      // subset of `all_buses`; "impossibility" due to contradicting assignment
  std::list<Config::Bus::Ptr> ambiguous_buses; // subset of `possible_buses`

  Port(NonPortDataPtr const&);

  bool isBusUnique(Config::Bus::Ptr const&) const;
};

} // namespace Technology_Adapter::Modbus
