#include "ConfigJson.hpp"

#include <fstream>

namespace Technology_Adapter::Modbus::Config {

using List = std::vector<json>;

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
  return RegisterRange( //
      json.at("begin").get<RegisterIndex>(),
      json.at("end").get<RegisterIndex>());
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
            raw |= register_value << shift;
            // NOLINTNEXTLINE(readability-magic-numbers)
            shift += 16;
          }
          return ((double)raw) * factor + offset;
        },
        Information_Model::DataType::DOUBLE,
    };
  } else {
    throw std::runtime_error("Unsupported decoder type " + type);
  }
}

Readable ReadableOfJson(json const& json) {
  auto decoder = DecoderOfJson(json.at("decoder"));

  return Readable( //
      json.at("name").get<std::string>(), //
      json.at("description").get<std::string>(), //
      decoder.return_type, //
      json.at("registers").get<std::vector<int>>(), //
      decoder.decoder);
}

/*
  Fills `readables` and `subgroups` according to the expectations of, both,
  `GroupOfJson` and `DeviceOfJson`
*/
void fillGroupFromJson(Group& group, json const& json) {
  auto const& elements = json.at("elements").get_ref<List const&>();
  for (auto const& element : elements) {
    auto const& type = element.at("element_type").get_ref<std::string const&>();
    if (type == "readable") {
      group.readables.push_back(ReadableOfJson(element));
    } else if (type == "group") {
      group.subgroups.push_back(GroupOfJson(element));
    } else {
      throw std::runtime_error("Unsupported element type " + type);
    }
  }
}

Group GroupOfJson(json const& json) {
  Group group( //
      json.at("name").get<std::string>(),
      json.at("description").get<std::string>());

  fillGroupFromJson(group, json);

  return group;
}

Device DeviceOfJson(json const& json) {
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

  Device device( //
      json.at("id").get<std::string>(), //
      json.at("name").get<std::string>(), //
      json.at("description").get<std::string>(), //
      json.at("slave_id").get<int>(), //
      json.at("burst_size").get<int>(), //
      holding_registers, input_registers);

  fillGroupFromJson(device, json);

  return device;
}

Bus BusOfJson(json const& json) {
  Bus bus( //
      json.at("possible_serial_ports").get<std::vector<std::string>>(), //
      json.at("baud").get<int>(), //
      ParityOfJson(json.at("parity")), //
      json.at("data_bits").get<int>(), //
      json.at("stop_bits").get<int>());

  auto const& devices = json.at("devices").get_ref<List const&>();
  for (auto const& device : devices) {
    bus.devices.push_back(DeviceOfJson(device));
  }

  return bus;
}

Buses BusesOfJson(json const& json) {
  Buses buses;
  auto const& buses_json = json.get_ref<List const&>();
  for (auto const& bus_json : buses_json) {
    buses.push_back(BusOfJson(bus_json));
  }
  return buses;
}

Buses loadConfig(std::string const& file_path) {
  std::ifstream input_stream(file_path);
  if (!input_stream) {
    throw std::runtime_error("Could not open " + file_path);
  }
  nlohmann::json json;
  input_stream >> json;
  return BusesOfJson(json);
}

} // namespace Technology_Adapter::Modbus::Config
