#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP

#include <functional>
#include <string>

#include <Information_Model/DataVariant.hpp>

namespace Modbus_Technology_Adapter {
namespace Config {

struct Readable {
  std::string name;
  std::string description;
  Information_Model::DataType type;

  /// Indices of the Modbus register to decode from
  std::vector<int> registers;

  /**
   * @pre The `vector` has the same length as `registers`
   * @post The `DataVariant` conforms to `type`
   */
  std::function<Information_Model::DataVariant(std::vector<uint16_t> const&)>
    decode;

  Readable(std::string const& /*name*/, std::string const& /*description*/,
      Information_Model::DataType, std::vector<int> const&,
      std::function<
          Information_Model::DataVariant(std::vector<uint16_t> const&)>
          const&);
};

struct Group {
  std::string name;
  std::string description;
  std::vector<Readable> readables;
  std::vector<Group> subgroups;

  Group(std::string const& /*name*/, std::string const& /*description*/);
};

struct Device : public Group {
  /// In the sense of `Information_Model::NamedElement`
  std::string id;
  int slave_id;

  /// Number of Modbus registers that may be read at once
  size_t burst_size;

  Device(std::string const& /*id*/, //
    std::string const& /*name*/, std::string const& /*description*/, //
    int /*slave_id*/, size_t /*burst_size*/);
};

} // namespace Config
} // namespace Modbus_Technology_Adapter

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_HPP
