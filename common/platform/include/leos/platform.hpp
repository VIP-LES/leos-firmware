#pragma once

#include "leos/cyphal/node.hpp"
#include "leos/log.h"

namespace leos::platform {

// Initialize console logging at the requested minimum severity.
bool init_log(ulog_level_t level = ULOG_INFO_LEVEL) noexcept;

// Initialize logging and the board status LED. A logging failure does not
// prevent the board from running.
void init(ulog_level_t log_level = ULOG_INFO_LEVEL) noexcept;

// Finish board setup without a Cyphal node (for example, if CAN is offline).
void setup_complete() noexcept;

// Finish setup with a live node: set its heartbeat state and light the LED.
// Pass a degraded health value when startup found a nonfatal problem.
void setup_complete(cyphal::Node& node,
                    cyphal::Node::Health health = cyphal::Node::Health::Nominal) noexcept;

// Signal an unrecoverable startup failure with a rapid LED blink. Call init()
// first so the LED pin is configured as an output.
[[noreturn]] void fatal() noexcept;

} // namespace leos::platform
