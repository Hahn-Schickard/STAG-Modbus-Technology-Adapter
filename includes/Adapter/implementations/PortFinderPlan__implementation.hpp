
namespace Technology_Adapter::Modbus {

namespace Internal_ {

} // namespace Internal_

struct PortFinderPlan::Port {
  bool assigned = false;
  std::list<Config::Bus::Ptr> possible_buses;
  std::list<Config::Bus::Ptr> ambiguous_buses; // subset of `possible_buses`

  bool isBusUnique(Config::Bus::Ptr const&) const;
};

} // namespace Technology_Adapter::Modbus
