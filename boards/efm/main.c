#include "pico/stdlib.h"
#include "common/platform.h"
#include "leos/mcp342x.h"

#define EFM1_ADC_ADDR 0b1101000
#define EFM2_ADC_ADDR 0b1101100
#define I2C_BLOCK i2c0
#define SDA 8
#define SCL 9

// from efm.c
void efm_task(mcp342x_dev_t *adc1, mcp342x_dev_t *adc2);

void main() {
    // --- INITIALIZE MODULE ---
    board_health_t health = BOARD_HEALTH_NOMINAL;

    if (leos_board_init() < 0) {
        LOG_ERROR("A critical communications error has occured. This node is offline.");
        leos_fatal();
    }
    sleep_ms(5000);

    mcp342x_dev_t adc1;
    if (mcp342x_init(&adc1, I2C_BLOCK, SDA, SCL, EFM1_ADC_ADDR) < 0) {
        LOG_ERROR("Failed to initialize ADC 1 on address %X.", EFM1_ADC_ADDR);
    }
    mcp342x_dev_t adc2;
    if (mcp342x_init(&adc2, I2C_BLOCK, SDA, SCL, EFM2_ADC_ADDR) < 0) {
        LOG_ERROR("Failed to initialize ADC 2 on address %X.", EFM2_ADC_ADDR);
    }
    mcp342x_set_resolution(&adc1, MCP342X_RES_12);
    mcp342x_set_resolution(&adc2, MCP342X_RES_12);

    leos_board_finish_setup(health);

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        leos_net_task();
        efm_task(&adc1, &adc2);
    }
}
