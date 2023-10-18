#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_JSON_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_JSON_HPP

#include <nlohmann/json.hpp>

#include "Config.hpp"

namespace Technology_Adapter::Modbus::Config {

using json = nlohmann::json;

/**
 * @brief Parse a `Parity` from JSON
 *
 * `json` is expected to be one of `"Even"`, `"Odd"`, or `"None"`.
 */
LibModbus::Parity ParityOfJson(json const& json);

/**
 * @brief Parse a `RegisterRange` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"start"` and `"end"` of JSON type `number`
 */
RegisterRange RegisterRangeOfJson(json const& json);

struct TypedDecoder {
  Readable::Decoder decoder;
  Information_Model::DataType return_type;
};

/**
 * @brief Parse a `TypedDecoder` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"type"` with value `"linear"`
 * - `"factor"` and `"offset"` of JSON type `number`
 *
 * The decoder converts the given registers to a `double` in an unsigned
 * little endian way and applies the given linear transformation to it.
 */
TypedDecoder DecoderOfJson(json const& json);

/**
 * @brief Parse a `Readable` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"name"` and `"description"` of JSON type `string`
 * - `"data_type"` as expected by `DataTypeOfJson`
 * - `"registers"` of JSON type `array` with entries of JSON type `number`
 * - `"decoder"` as expected by `DecoderOfJson`
 */
Readable ReadableOfJson(json const& json);

/**
 * @brief Parse a `Group` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `elements` of JSON type array.
 *   The entries are expected to be of JSON type object with a field
 *   `element_type`. Furthermore, one of the following is expected:
 *   - The field has value `readable` and the object is as expected by
 *     `ReadableOfJson`
 *   - The field has value `group` and the object is as expected by this
 *     function
 */
Group GroupOfJson(json const& json);

/**
 * @brief Parse a `Device` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"id"`, `"name"`, and `"description"` of JSON type `string`
 * - `"slave_id"` and `"burst_size"` of JSON type `number`
 * - optionally `max_retries` of JSON type `number` with default `3`
 * - optionally `retry_delay` of JSON type `number` with default `0`
 * - `"readable_registers"` of JSON type array with entries as expected by
 *   `RegisterRangeOfJson`
 * - `elements` of JSON type array.
 *   The entries are expected to be of JSON type object with a field
 *   `element_type`. Furthermore, one of the following is expected:
 *   - The field has value `readable` and the object is as expected by
 *     `ReadableOfJson`
 *   - The field has value `group` and the object is as expected by
 *     `GroupOfJson`
 */
Device DeviceOfJson(json const& json);

/**
 * @brief Parse a `Bus` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"possible_serial_ports"` of JSON type `array` with entries of JSON type
 *   `string`
 * - `"baud"`, `"data_bits"`, and `"stop_bits"` of JSON type `number`
 * - `"parity"` as expected by `ParityOfJson`
 * - `"devices"` of JSON type `array` with entries as expected by `DeviceOfJson`
 */
Bus BusOfJson(json const& json);

/**
 * @brief Parse `Buses` from JSON
 *
 * `json` is expected to be a JSON array with entries as expected by `BusOfJson`
 */
Buses BusesOfJson(json const& json);

Buses loadConfig(std::string const& file_path);

} // namespace Technology_Adapter::Modbus::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_JSON_HPP
