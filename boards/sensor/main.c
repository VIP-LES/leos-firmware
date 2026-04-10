#include "pico/stdlib.h"
#include "common/platform.h"

#include "purpleboard.h"
#include "cutdown.h"

void main() {
    // --- INITIALIZE MODULE ---
    board_health_t health = BOARD_HEALTH_NOMINAL;

    if (leos_board_init() < 0) {
        LOG_ERROR("A critical communications error has occured. This node is offline.");
        leos_fatal();
    }

    // Setup purpleboard
    leos_purpleboard_result_t pb_err = leos_purpleboard_init(PB_I2C_BLOCK, PB_PIN_SDA, PB_PIN_SCL, &pb);
    if (pb_err == PB_SENSOR_NO_DETECT) {
        LOG_ERROR("The purpleboard sensors failed to initialize, not detected.");
        // Set health to caution - sensors are not critical devices.
        health = BOARD_HEALTH_CAUTION;
    } else {
        pb_initialized = true;
    }


    leos_board_finish_setup(health);

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        leos_net_task();
        purpleboard_task();
        cutdown_task();
    }
}