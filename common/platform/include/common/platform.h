#pragma once
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "leos/cyphal/node.h"
#include "common/platform_enums.h"

extern MCP251XFD leos_can;
extern leos_cyphal_node_t leos_node;

// Initializes common components on all LEOS stack boards
int leos_board_init();
// TODO: Change from int to custom return type

// Runs service routines for CAN bus and Cyphal handling
void leos_net_task();

//  An unrecoverable error has occured
void leos_fatal();

// Call this when you are finished with board setup and will run the main loop
void leos_board_finish_setup(board_health_t health);