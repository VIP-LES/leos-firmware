# Common Setup Code for LEOS Stack Modules

This folder holds setup shared by the firmware boards. `can` owns the reusable
MCP251XFD hardware and controller configuration. `platform` provides the C++
`leos::platform::init()` and `leos::platform::fatal()` helpers. Each board
owns its Cyphal node and initializes it with the shared CAN configuration.
Board applications can include `leos/firmware.hpp` for the common Pico, Cyphal,
logging, configuration, and platform APIs.
