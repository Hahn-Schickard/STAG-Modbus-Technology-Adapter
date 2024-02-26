#include "internal/ConfigJson.hpp"

#include <fstream>

#include "HSCUL/FloatingPoint.hpp"

namespace Technology_Adapter::Modbus::Config {

using List = std::vector<json>;

namespace {

// Lifts conversion from `std::string` to `ConstString` to vectors
std::vector<ConstString::ConstString> constStringVector(
    std::vector<std::string> const& strings) {

  std::vector<ConstString::ConstString> result;
  result.reserve(strings.size());
  for (auto const& string : strings) {
    result.emplace_back(string);
  }
  return result;
}

// Like `json.at(field_name).get<T>()`, but with `default_value`
template <class T>
T readWithDefault(json const& json, char const* field_name, T default_value) {
  return json.count(field_name) > 0 //
      ? json.at(field_name).get<T>()
      : default_value;
}

// NOLINTNEXTLINE(cert-err58-cpp)
TypedDecoder float_decoder{
    [](std::vector<uint16_t> const& registers)
        -> Information_Model::DataVariant {
      //
      // Convert 16-bit registers to bytes
      size_t num_registers = registers.size();
      std::vector<uint8_t> bytes;
      bytes.reserve(2 * num_registers);
      for (auto register_value : registers) {
        bytes.push_back(register_value);
        // NOLINTNEXTLINE(readability-magic-numbers)
        bytes.push_back(register_value >> 8);
      }

      switch (num_registers) {
      case 2:
        return HSCUL::toFloat(bytes, HSCUL::ByteOrder::LSB_First);
      case 4:
        return HSCUL::toDouble(bytes, HSCUL::ByteOrder::LSB_First);
      default:
        throw std::runtime_error(
            "In float decoder: Unsupported size for IEEE 754");
      }
    },
    Information_Model::DataType::DOUBLE,
};

} // namespace

LibModbus::Parity ParityOfJson(json const& json) {
  auto const& name = json.get_ref<std::string const&>();
  if (name == "Even") {
    return LibModbus::Parity::Even;
  } else if (name == "Odd") {
    return LibModbus::Parity::Odd;
  } else if (name == "None") {
    return LibModbus::Parity::None;
  } else {
    throw std::runtime_error("Could not parse parity " + name);
  }
}

RegisterRange RegisterRangeOfJson(json const& json) {
  return RegisterRange{
      json.at("begin").get<RegisterIndex>(),
      json.at("end").get<RegisterIndex>(),
  };
}

TypedDecoder DecoderOfJson(json const& json) {
  auto const& type = json.at("type").get_ref<std::string const&>();
  if (type == "linear") {
    double factor = json.at("factor").get<double>();
    double offset = json.at("offset").get<double>();
    return {
        [factor, offset](std::vector<uint16_t> const& register_values) {
          uint64_t raw = 0;
          unsigned shift = 0;
          for (uint16_t register_value : register_values) {
            raw |= ((uint64_t)register_value) << shift;
            // NOLINTNEXTLINE(readability-magic-numbers)
            shift += 16;
          }
          return ((double)raw) * factor + offset;
        },
        Information_Model::DataType::DOUBLE,
    };
  } else if (type == "float") {
    return float_decoder;
  } else {
    throw std::runtime_error("Unsupported decoder type " + type);
  }
}

Readable ReadableOfJson(json const& json) {
  auto decoder = DecoderOfJson(json.at("decoder"));

  return Readable{
      ConstString::ConstString(json.at("name").get<std::string>()), //
      ConstString::ConstString(json.at("description").get<std::string>()), //
      decoder.return_type, //
      json.at("registers").get<std::vector<int>>(), //
      decoder.decoder,
  };
}

/*
  Extracts `Readable`s from `json` using `ReadableOfJson`.

  `json` is expected to be as for `GroupOfJson`.

  @throws `std::runtime_error
  @throws whatever `nlohmann/json` throws
*/
std::vector<Readable> readablesOfJson(json const& json) {
  std::vector<Readable> readables;
  auto const& elements = json.at("elements").get_ref<List const&>();
  for (auto const& element : elements) {
    auto const& type = element.at("element_type").get_ref<std::string const&>();
    if (type == "readable") {
      readables.push_back(ReadableOfJson(element));
    } else if (type == "group") {
    } else {
      throw std::runtime_error("Unsupported element type " + type);
    }
  }
  return readables;
}

/*
  Extracts `Group`s from `json` using `ReadableOfJson`.

  `json` is expected to be as for `GroupOfJson`.

  @throws `std::runtime_error
  @throws whatever `nlohmann/json` throws
*/
std::vector<Group> subgroupsOfJson(json const& json) {
  std::vector<Group> subgroups;
  auto const& elements = json.at("elements").get_ref<List const&>();
  for (auto const& element : elements) {
    auto const& type = element.at("element_type").get_ref<std::string const&>();
    if (type == "readable") {
    } else if (type == "group") {
      subgroups.push_back(GroupOfJson(element));
    } else {
      throw std::runtime_error("Unsupported element type " + type);
    }
  }
  return subgroups;
}

Group GroupOfJson(json const& json) {
  return Group{
      ConstString::ConstString(json.at("name").get<std::string>()),
      ConstString::ConstString(json.at("description").get<std::string>()),
      readablesOfJson(json),
      subgroupsOfJson(json),
  };
}

Device::NonemptyPtr DeviceOfJson(json const& json) {
  auto const& holding_registers_json =
      json.at("holding_registers").get_ref<List const&>();
  std::vector<RegisterRange> holding_registers;
  holding_registers.reserve(holding_registers_json.size());
  for (auto const& range : holding_registers_json) {
    holding_registers.push_back(RegisterRangeOfJson(range));
  }

  auto const& input_registers_json =
      json.at("input_registers").get_ref<List const&>();
  std::vector<RegisterRange> input_registers;
  input_registers.reserve(input_registers_json.size());
  for (auto const& range : input_registers_json) {
    input_registers.push_back(RegisterRangeOfJson(range));
  }

  return Device::NonemptyPtr::make( //
      ConstString::ConstString(json.at("id").get<std::string>()), //
      ConstString::ConstString(json.at("name").get<std::string>()), //
      ConstString::ConstString(json.at("description").get<std::string>()), //
      readablesOfJson(json), subgroupsOfJson(json),
      json.at("slave_id").get<int>(), //
      json.at("burst_size").get<size_t>(), //
      readWithDefault<size_t>(json, "max_retries", 3), //
      readWithDefault<size_t>(json, "retry_delay", 0), //
      holding_registers, input_registers);
}

Bus::NonemptyPtr BusOfJson(json const& json) {
  std::vector<Device::NonemptyPtr> devices;
  auto const& devices_json = json.at("devices").get_ref<List const&>();
  devices.reserve(devices_json.size());
  for (auto const& device : devices_json) {
    devices.push_back(DeviceOfJson(device));
  }

  return Bus::NonemptyPtr::make( //
      constStringVector(
          json.at("possible_serial_ports").get<std::vector<std::string>>()),
      json.at("baud").get<int>(), //
      ParityOfJson(json.at("parity")), //
      json.at("data_bits").get<int>(), //
      json.at("stop_bits").get<int>(), //
      devices);
}

Buses BusesOfJson(json const& json) {
  Buses buses;
  auto const& buses_json = json.get_ref<List const&>();
  for (auto const& bus_json : buses_json) {
    buses.push_back(BusOfJson(bus_json));
  }
  return buses;
}

Buses loadConfig(ConstString::ConstString const& file_path) {
  std::ifstream input_stream(file_path.c_str());
  if (!input_stream) {
    throw std::runtime_error(("Could not open " + file_path).c_str());
  }
  nlohmann::json json;
  input_stream >> json;
  return BusesOfJson(json);
}

} // namespace Technology_Adapter::Modbus::Config
