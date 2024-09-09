# Modbus Technology Adapter

The present Technology Adapter supports reading from Modbus RTU devices.

## Configuration

The syntax of the configuration file is defined in the interface
`includes/Adapter/internal/ConfigJson.hpp`.
More precisely, the file must be in JSON format as parsable by the function
`Technology_Adapter::Modbus::Config::BusesOfJson`.

The semantics of the configuration file are defined in the interface
`includes/Adapter/internal/Config.hpp`,
by the return type `Technology_Adapter::Modbus::Config::Buses`
of the above function.

## Further considerations

The user that runs the STAG process needs to have access rights
for the device files that correspond to the hardware buses.
For example, on Debian the device `/dev/ttyUSB0` is only accessable
by user `root` or group `dialout`.
In this case, one needs to add the STAG user to this group.
This can be achieved by editing `/etc/group` or with the `adduser` program.
