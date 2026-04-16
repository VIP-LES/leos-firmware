#include "pico/stdlib.h"
#include "config.h"
#include "cyphal_bridge.h"
#include "radio.h"
#include "common/platform.h"
#include "canard.h"
#include "hardware/irq.h"

void irq_cb(uint gpio, uint32_t event_mask) {
    LOG_TRACE("radio irq_cb gpio=%u events=0x%08lx", (unsigned)gpio, (unsigned long)event_mask);

    switch (gpio) {
        case LEOS_SX1262_PIN_DIO1:
            LOG_TRACE("radio irq_cb routing SX1262 DIO1");
            radio_handle_dio1_irq_sx1262();
            break;
        case LEOS_SX1268_PIN_DIO1:
            LOG_TRACE("radio irq_cb routing SX1268 DIO1");
            radio_handle_dio1_irq_sx1268();
            break;
        default:
            LOG_TRACE("radio irq_cb forwarding non-radio gpio=%u", (unsigned)gpio);
            break;
    }

    leos_mcp251xfd_irq_handler(gpio, event_mask);
}

void main() {
    
    // --- INITIALIZE MODULE ---
    board_health_t health = BOARD_HEALTH_NOMINAL;

    if (leos_board_init() < 0) {
        LOG_ERROR("A critical communications error has occured. This node is offline.");
        leos_fatal();
    }


    if (radio_init() < 0) {
        LOG_ERROR("Failed to initialize configured radios. This node will now panic.");
        leos_fatal();
    }

    LOG_INFO("Installing shared GPIO IRQ callback for radio board");
    gpio_set_irq_callback(irq_cb);

    LOG_INFO("Enabling DIO1 IRQs: sx1262 gpio=%u level=%u, sx1268 gpio=%u level=%u",
             (unsigned)LEOS_SX1262_PIN_DIO1,
             (unsigned)gpio_get(LEOS_SX1262_PIN_DIO1),
             (unsigned)LEOS_SX1268_PIN_DIO1,
             (unsigned)gpio_get(LEOS_SX1268_PIN_DIO1));
    gpio_set_irq_enabled(LEOS_SX1262_PIN_DIO1, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(LEOS_SX1268_PIN_DIO1, GPIO_IRQ_EDGE_RISE, true);

    LOG_INFO("DIO1 IRQs armed: sx1262 gpio=%u level=%u, sx1268 gpio=%u level=%u",
             (unsigned)LEOS_SX1262_PIN_DIO1,
             (unsigned)gpio_get(LEOS_SX1262_PIN_DIO1),
             (unsigned)LEOS_SX1268_PIN_DIO1,
             (unsigned)gpio_get(LEOS_SX1268_PIN_DIO1));
      

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
