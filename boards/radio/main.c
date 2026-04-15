#include "pico/stdlib.h"
#include "config.h"
#include "cyphal_bridge.h"
#include "radio.h"
#include "common/platform.h"
#include "canard.h"

void main() {
    
    // --- INITIALIZE MODULE ---
    board_health_t health = BOARD_HEALTH_NOMINAL;

    if (leos_board_init() < 0) {
        LOG_ERROR("A critical communications error has occured. This node is offline.");
        leos_fatal();
    }


    if (!radio_init()) {
        LOG_ERROR("Failed to initialize configured radios. This node will now panic.");
        leos_fatal();
    }
    
    /* Register Cyphal subscriptions. All callback bodies live in
     * cyphal_bridge.c — main.c is the single place where the
     * application's message-level behavior is visible. */

    leos_cyphal_result_t sub_rc;

    sub_rc = leos_cyphal_subscribe(
        &leos_node,
        CanardTransferKindMessage,
        CYPHAL_SUB_SENSOR_GPS_ID,
        CYPHAL_SUB_SENSOR_GPS_EXTENT,
        cyphal_bridge_on_sensor_gps,
        NULL);

    if (sub_rc != LEOS_CYPHAL_OK)
    {
        /* Non-fatal: log and continue. The board can still forward EFM
         * traffic even if this subscription fails. */
        /* LOG_ERROR("Failed to subscribe sensor_gps: %d", sub_rc); */
    }

    sub_rc = leos_cyphal_subscribe(
        &leos_node,
        CanardTransferKindMessage,
        CYPHAL_SUB_EFM_ID,
        CYPHAL_SUB_EFM_EXTENT,
        cyphal_bridge_on_efm,
        NULL);

    if (sub_rc != LEOS_CYPHAL_OK)
    {
        /* LOG_ERROR("Failed to subscribe efm: %d", sub_rc); */
    }

    leos_board_finish_setup(health);
    LOG_INFO("Entering main loop...");
    while (true) {
        leos_net_task();
        radio_service_irqs();

        if (radio_sx1262_packet_available()) {
            cyphal_bridge_publish_sx1262_rx(&leos_node);
        }

    }
}
