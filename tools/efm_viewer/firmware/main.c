#include "pico/stdlib.h"
#include "leos/mcp342x.h"
#include "leos/log.h"
#include <stdio.h>

#define EFM1_ADC_ADDR 0b1101000
#define EFM2_ADC_ADDR 0b1101100
#define I2C_BLOCK i2c0
#define SDA 8
#define SCL 9

void efm_task(mcp342x_dev_t *adc1, mcp342x_dev_t *adc2);

void main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

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

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        efm_task(&adc1, &adc2);
    }
}

void efm_task(mcp342x_dev_t *adc1, mcp342x_dev_t *adc2) {
    for (int ch = 0; ch < 4; ch+=3) {
        mcp342x_channel_t channel = (mcp342x_channel_t)ch;

        float voltage1;
        float voltage2;

        // --- ADC 1 ---
        mcp342x_set_channel(adc1, channel);

        if (mcp342x_read_voltage(adc1, &voltage1) < 0) {
            LOG_WARNING("Failed to read ADC1 voltage.");
        } else {
            printf("{\"adc\": 1, \"channel\": %d, \"voltage\": %.06F}\n", ch + 1, voltage1);
        }


        // --- ADC 2 ---
        mcp342x_set_channel(adc2, channel);

        if (mcp342x_read_voltage(adc2, &voltage2) < 0){
            LOG_WARNING("Failed to read ADC2 voltage.");
        } else {
            printf("{\"adc\": 2, \"channel\": %d, \"voltage\": %.06F}\n", ch + 1, voltage2);
        }


    }
}