#include "config.h"
#include <string.h>

void config_build_sx1262(leos_radio_config_t *cfg) {
    if (cfg == NULL) return;

    leos_sx126x_get_default_config(LEOS_RADIO_SX1262, cfg);
    cfg->rf_frequency_hz = SX1262_RF_FREQUENCY_HZ;
    cfg->tx_timeout_ms = SX1262_TX_TIMEOUT_MS;
    cfg->tx_power_dbm = SX1262_TX_POWER_DBM;
    cfg->crc_enabled = SX1262_CRC_ENABLED;
    cfg->iq_inverted = SX1262_IQ_INVERTED;
    cfg->bandwidth = SX1262_BANDWIDTH;
    cfg->coding_rate = SX1262_CODING_RATE;
    cfg->spreading_factor = SX1262_SPREADING_FACTOR;
    cfg->sync_word = SX1262_SYNC_WORD;
    cfg->dio2_rf_switch_enable = SX1262_DIO2_RF_SWITCH_ENABLE;
    cfg->dio3_tcxo_enable = SX1262_DIO3_TCXO_ENABLE;
    cfg->tcxo_voltage = SX1262_TCXO_VOLTAGE;
    cfg->tcxo_delay_us = SX1262_TCXO_DELAY_US;
}

void config_build_sx1268(leos_radio_config_t *cfg) {
    if (cfg == NULL) return;

    leos_sx126x_get_default_config(LEOS_RADIO_SX1268, cfg);
    cfg->rf_frequency_hz = SX1268_RF_FREQUENCY_HZ;
    cfg->tx_timeout_ms = SX1268_TX_TIMEOUT_MS;
    cfg->tx_power_dbm = SX1268_TX_POWER_DBM;
    cfg->crc_enabled = SX1268_CRC_ENABLED;
    cfg->iq_inverted = SX1268_IQ_INVERTED;
    cfg->bandwidth = SX1268_BANDWIDTH;
    cfg->coding_rate = SX1268_CODING_RATE;
    cfg->spreading_factor = SX1268_SPREADING_FACTOR;
    cfg->sync_word = SX1268_SYNC_WORD;
    cfg->dio2_rf_switch_enable = SX1268_DIO2_RF_SWITCH_ENABLE;
    cfg->dio3_tcxo_enable = SX1268_DIO3_TCXO_ENABLE;
    cfg->tcxo_voltage = SX1268_TCXO_VOLTAGE;
    cfg->tcxo_delay_us = SX1268_TCXO_DELAY_US;
}

static void config_build_shared_sx126x_hw(leos_radio_hw_config_t *cfg) {
    if (cfg == NULL) return;

    cfg->platform_spi = (void *)LEOS_SX126X_SPI_PORT;
    cfg->spi_baud_hz = LEOS_SX126X_SPI_BAUD_HZ;
    cfg->pin_sck = LEOS_SX126X_PIN_SCK;
    cfg->pin_mosi = LEOS_SX126X_PIN_MOSI;
    cfg->pin_miso = LEOS_SX126X_PIN_MISO;
}

void config_build_sx1262_hw(leos_radio_hw_config_t *cfg) {
    if (cfg == NULL) return;

    memset(cfg, 0, sizeof(*cfg));
    config_build_shared_sx126x_hw(cfg);
    cfg->pin_nss = LEOS_SX1262_PIN_NSS;
    cfg->pin_busy = LEOS_SX1262_PIN_BUSY;
    cfg->pin_reset = LEOS_SX1262_PIN_RESET;
    cfg->pin_dio1 = LEOS_SX1262_PIN_DIO1;
}

void config_build_sx1268_hw(leos_radio_hw_config_t *cfg) {
    if (cfg == NULL) return;

    memset(cfg, 0, sizeof(*cfg));
    config_build_shared_sx126x_hw(cfg);
    cfg->pin_nss = LEOS_SX1268_PIN_NSS;
    cfg->pin_busy = LEOS_SX1268_PIN_BUSY;
    cfg->pin_reset = LEOS_SX1268_PIN_RESET;
    cfg->pin_dio1 = LEOS_SX1268_PIN_DIO1;
}
