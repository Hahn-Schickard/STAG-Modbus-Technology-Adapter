# Changelog

## [0.2.2] - 2023-10-11

### Fixed
- Handling of bus timeouts

## [0.2.1] - 2023-10-10

### Added
- `ModbusTechnologyAdapter(std::string const& config_path)`

### Fixed
- Outdated documentation of JSON format

## [0.2.0] - 2023-09-21

### Added
- `begin()` and `end()` to `RegisterSet`
- Port detection
- Logging
- Retries for read operations
- Restartability of `ModbusTechnologyAdapter`

### Changed
- Technology Adapter Interface dependency to 0.2
- Config pointers to NonemptyPtr
- JSON format
- Implementation of `ModbusError` to be thread-safe

### Removed
- Module `Index` in favor of external project

## [0.1.0] - 2023-07-12

### Added
- module `ConfigJson`
- support for input registers

### Changed
- namespaces
- trigger for device registration
- Threadsafe_Containers dependency to 0.8

## [0.0.0] - 2023-05-08
