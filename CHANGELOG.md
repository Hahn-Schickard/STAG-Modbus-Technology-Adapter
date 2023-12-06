# Changelog
## [0.2.9] - xxxx-xx-xx

### Changed
- Use implicit `ConstString(char const*)` from ConstString 0.1.1

### Fixed
- Bug where out-of-candidates searches weren't restarted
- Eliminated capture-by-reference for lambdas that go to other threads

## [0.2.8] - 2023-11-21

### Added
- More code documentation

### Fixed
- Eliminated a deadlock

## [0.2.7] - 2023-10-30

### Added
- Unit tests for Port and Bus

### Changed
- Hide more implementation details
- Replaced embedded copies with pointers in Config

### Fixed
- Deadlock when registration fails

## [0.2.6] - 2023-10-25

### Changed
- Replaced `std::string` with `ConstString` where feasible

### Fixed
- Bug where buses were registered before being started

## [0.2.5] - 2023-10-18

### Added
- Config option for number of retries
- Config option for retry delay

### Fixed
- Buggy behaviour when reading fails during metric creation

## [0.2.4] - 2023-10-13

### Fixed
- Buggy re-discovery

## [0.2.3] - 2023-10-12

### Changed
- Enabled failed buses for re-discovery

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
