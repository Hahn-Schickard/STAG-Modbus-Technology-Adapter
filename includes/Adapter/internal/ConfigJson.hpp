#ifndef _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_JSON_HPP
#define _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_JSON_HPP

/**
 * This module provides parsing of `Config::` types from JSON
 */

#include <nlohmann/json.hpp>

#include "Config.hpp"

namespace Technology_Adapter::Modbus::Config {

using json = nlohmann::json;

/**
 * @brief Parse a `Parity` from JSON
 *
 * `json` is expected to be one of `"Even"`, `"Odd"`, or `"None"`.
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
LibModbus::Parity ParityOfJson(json const& json);

/**
 * @brief Parse a `RegisterRange` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"begin"` and `"end"` of JSON type `number`
 *
 * @throws whatever `nlohmann/json` throws
 */
RegisterRange RegisterRangeOfJson(json const& json);

struct TypedDecoder {
  Readable::Decoder decoder;
  Information_Model::DataType return_type;
};

/**
 * @brief Parse a `TypedDecoder` from JSON
 *
 * `json` is expected to be a JSON object with a field `"type"`. Furthermore,
 * one of the following must hold:
 *
 * - `"type"` has value `"linear"` and there are further fields `"factor"` and
 *   `"offset"` of JSON type `number`.
 *   The decoder converts the given registers to an unsigned integer in a little
 *   endian way and then applies the given linear transformation.
 *
 * - `"type"` has value `"float"` and there are exactly two registers.
 *   The decoder treats the registers as an IEEE 754 Single, given in little
 *   endian.
 *
 * - `"type"` has value `"float"` and there are exactly four registers.
 *   The decoder treats the registers as an IEEE 754 Double, given in little
 *   endian.
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
TypedDecoder DecoderOfJson(json const& json);

/**
 * @brief Parse a `Readable` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"name"` and `"description"` of JSON type `string`
 * - `"registers"` of JSON type `array` with entries of JSON type `number`
 * - `"decoder"` as expected by `DecoderOfJson`
 *
 * The `Readable::type` is implicit from the decoder.
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
Readable ReadableOfJson(json const& json);

/**
 * @brief Parse a `Group` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"name"` and `"description"` of JSON type `string`
 * - `"elements"` of JSON type `array`.
 *   The entries are expected to be of JSON type `object` with a field
 *   `element_type`. Furthermore, one of the following is expected:
 *   - The field has value `readable` and the object is as expected by
 *     `ReadableOfJson`
 *   - The field has value `group` and the object is as expected by this
 *     function
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
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
 * - `"holding_registers"` and `"input_registers"` of JSON type `array` with
 *    entries as expected by `RegisterRangeOfJson`
 * - `elements` of JSON type `array`.
 *   The entries are expected to be of JSON type object with a field
 *   `element_type`. Furthermore, one of the following is expected:
 *   - The field has value `readable` and the object is as expected by
 *     `ReadableOfJson`
 *   - The field has value `group` and the object is as expected by
 *     `GroupOfJson`
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
Device::NonemptyPtr DeviceOfJson(json const& json);

/**
 * @brief Parse a `Bus` from JSON
 *
 * `json` is expected to be a JSON object with fields
 * - `"possible_serial_ports"` of JSON type `array` with entries of JSON type
 *   `string`
 * - `"baud"`, `"data_bits"`, and `"stop_bits"` of JSON type `number`
 * - `"parity"` as expected by `ParityOfJson`
 * - `"devices"` of JSON type `array` with entries as expected by `DeviceOfJson`
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
Bus::NonemptyPtr BusOfJson(json const& json);

/**
 * @brief Parse `Buses` from JSON
 *
 * `json` is expected to be a JSON array with entries as expected by `BusOfJson`
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
Buses BusesOfJson(json const& json);

/**
 * @brief Applies `BusesOfJson` to the contents of the file at `file_path`
 *
 * @throws `std::runtime_error
 * @throws whatever `nlohmann/json` throws
 */
Buses loadConfig(ConstString::ConstString const& file_path);

} // namespace Technology_Adapter::Modbus::Config

#endif // _MODBUS_TECHNOLOGY_ADAPTER_CONFIG_JSON_HPP
