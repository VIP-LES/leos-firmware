#pragma once

// Common includes for every board target
#include "pico/stdlib.h"

#include "common/can_config.h"
#include "leos/cyphal/node.hpp"
#include "leos/cyphal/publisher.hpp"
#include "leos/cyphal/subscriber.hpp"
#include "leos/cyphal/service.hpp"
#include "leos/log.h"
#include "leos/platform.hpp"

namespace leos {

using Health = cyphal::Node::Health;
using Mode = cyphal::Node::Mode;

} // namespace leos
