#include "Config.hpp"

namespace Modbus_Technology_Adapter {
namespace Config {

Readable::Readable(std::string const& name_, std::string const& description_,
    Information_Model::DataType type_, std::vector<int> const& registers_,
    std::function<
        Information_Model::DataVariant(std::vector<uint16_t> const&)>
        const& decode_)
    : name(name_), description(description_), type(type_),
      registers(registers_), decode(decode_) {}

Group::Group(std::string const& name_, std::string const& description_)
    : name(name_), description(description_) {}

Device::Device(std::string const& id_, //
    std::string const& name, std::string const& description, //
    int slave_id_, size_t burst_size_)
    : Group(name,description), id(id_), slave_id(slave_id_),
      burst_size(burst_size_) {}

} // namespace Config
} // namespace Modbus_Technology_Adapter
