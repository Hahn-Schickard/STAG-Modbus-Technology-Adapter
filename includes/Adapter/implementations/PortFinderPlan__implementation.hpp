
#include <optional>

namespace Technology_Adapter::Modbus {

namespace Internal_ {

} // namespace Internal_

struct PortFinderPlan::Port {
  std::optional<Config::Bus::Ptr> assigned;
  std::list<Config::Bus::Ptr> all_buses;
  std::list<Config::Bus::Ptr> possible_buses;
      // subset of `all_buses`; "impossibility" due to contradicting assignment
  std::list<Config::Bus::Ptr> ambiguous_buses; // subset of `possible_buses`

  bool isBusUnique(Config::Bus::Ptr const&) const;
};

} // namespace Technology_Adapter::Modbus
