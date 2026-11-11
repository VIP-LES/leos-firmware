#pragma once

#include "leos/mcp251xfd/config.h"

// Shared CAN controller settings. FIFO and filter arrays have static storage,
// as required by Node::init() while it configures the MCP251XFD.
extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;
