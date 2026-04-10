#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "leos/cyphal/transport/mcp251xfd.h"
#include "leos/mcp251xfd/debug.h"
#include "leos/cyphal/node.h"
#include "pico/stdlib.h"
#include "leos/mcp251xfd/config.h"
#include "common/platform.h"

extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;

MCP251XFD leos_can;
leos_cyphal_node_t leos_node;

void mcp_read_pending_cb(MCP251XFD* dev, void *node_ref) {
    leos_cyphal_node_t *node = (leos_cyphal_node_t *)node_ref;
    leos_cyphal_rx_process(node);
}

int leos_board_init() {
    leos_log_init_console(ULOG_INFO_LEVEL);

    // Setup CANBus Communication
    eERRORRESULT err;
    err = leos_mcp251xfd_init(&leos_can, &can_hw_config, &can_config, true);
    if (err != ERR_OK) {
        LOG_ERROR("Failed to init MCP251XFD: %s", mcp251xfd_debug_error_reason(err));
        return -1;
    }
    leos_cyphal_transport_t transport = leos_cyphal_transport_mcp251xfd(&leos_can);
    leos_cyphal_result_t can_result;
    can_result = leos_cyphal_init(&leos_node, transport, 10);
    if (can_result != LEOS_CYPHAL_OK) {
        LOG_ERROR("Failed to initialize Cyphal/Libcanard: %d", can_result);
        return -2;
    }
    // Attach CANBus receive handler
    leos_mcp251xfd_set_rx_handler(&leos_can, mcp_read_pending_cb, &leos_node);

    // Setup board LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

void leos_net_task() {
    leos_mcp251xfd_task(&leos_can);
    leos_cyphal_task(&leos_node);
}

// Locks us in an infinite loop with a specific blink pattern
// Pattern: two quick flashes every 2 seconds
void leos_fatal() {
    while(true) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(250);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(250);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(250);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(2000);
    }
}

void leos_board_finish_setup(board_health_t health) {
    // Set health status
    leos_node.health.value = (uint8_t) health;
    // After finishing initialization, set our mode to operational
    leos_node.mode.value = uavcan_node_Mode_1_0_OPERATIONAL;
    // Turn on board LED to indicate setup success.
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
}