#include "radio.h"
#include "config.h"
#include "leos/log.h"
#include "hardware/gpio.h"

int radio_init() {
    leos_radio_hw_config_t sx1262_hw_cfg;
    leos_radio_hw_config_t sx1268_hw_cfg;
    leos_radio_config_t sx1262_cfg;
    leos_radio_config_t sx1268_cfg;

    config_build_sx1262_hw(&sx1262_hw_cfg);
    config_build_sx1268_hw(&sx1268_hw_cfg);
    config_build_sx1262(&sx1262_cfg);
    config_build_sx1268(&sx1268_cfg);

    LOG_INFO("Radio init config: sx1262 dio1=%u busy=%u nss=%u reset=%u freq=%lu tx_timeout_ms=%lu bw=%d sf=%d cr=%d sync=0x%04x",
             (unsigned)sx1262_hw_cfg.pin_dio1,
             (unsigned)sx1262_hw_cfg.pin_busy,
             (unsigned)sx1262_hw_cfg.pin_nss,
             (unsigned)sx1262_hw_cfg.pin_reset,
             (unsigned long)sx1262_cfg.rf_frequency_hz,
             (unsigned long)sx1262_cfg.tx_timeout_ms,
             (int)sx1262_cfg.bandwidth,
             (int)sx1262_cfg.spreading_factor,
             (int)sx1262_cfg.coding_rate,
             (unsigned)sx1262_cfg.sync_word);
    LOG_INFO("Radio aux config: sx1262 dio2_rf_switch=%d dio3_tcxo=%d tcxo_voltage=%d tcxo_delay_us=%lu",
             sx1262_cfg.dio2_rf_switch_enable ? 1 : 0,
             sx1262_cfg.dio3_tcxo_enable ? 1 : 0,
             (int)sx1262_cfg.tcxo_voltage,
             (unsigned long)sx1262_cfg.tcxo_delay_us);
    LOG_INFO("Radio init config: sx1268 dio1=%u busy=%u nss=%u reset=%u freq=%lu tx_timeout_ms=%lu bw=%d sf=%d cr=%d sync=0x%04x",
             (unsigned)sx1268_hw_cfg.pin_dio1,
             (unsigned)sx1268_hw_cfg.pin_busy,
             (unsigned)sx1268_hw_cfg.pin_nss,
             (unsigned)sx1268_hw_cfg.pin_reset,
             (unsigned long)sx1268_cfg.rf_frequency_hz,
             (unsigned long)sx1268_cfg.tx_timeout_ms,
             (int)sx1268_cfg.bandwidth,
             (int)sx1268_cfg.spreading_factor,
             (int)sx1268_cfg.coding_rate,
             (unsigned)sx1268_cfg.sync_word);
    LOG_INFO("Radio aux config: sx1268 dio2_rf_switch=%d dio3_tcxo=%d tcxo_voltage=%d tcxo_delay_us=%lu",
             sx1268_cfg.dio2_rf_switch_enable ? 1 : 0,
             sx1268_cfg.dio3_tcxo_enable ? 1 : 0,
             (int)sx1268_cfg.tcxo_voltage,
             (unsigned long)sx1268_cfg.tcxo_delay_us);

    leos_radio_status_t err = LEOS_RADIO_OK;
    if (RADIO_900MHZ_ENABLE) {
        err = leos_sx126x_init(LEOS_RADIO_SX1262, &sx1262_hw_cfg, &sx1262_cfg);
        if (err != LEOS_RADIO_OK) {
            LOG_ERROR("SX1262 900MHz radio init failed. Error code: %d", err);
            return -1;
        }
    }

    if (RADIO_400MHZ_ENABLE) {
        err = leos_sx126x_init(LEOS_RADIO_SX1268, &sx1268_hw_cfg, &sx1268_cfg);
        if (err != LEOS_RADIO_OK) {
            LOG_ERROR("SX1268 400MHz radio init failed. Error code: %d", err);
            return -2;
        }
    }

    LOG_INFO("Radios initialized");
    return 0;
}

void radio_handle_dio1_irq_sx1262() {
    LOG_TRACE("radio_handle_dio1_irq_sx1262 level=%u", (unsigned)gpio_get(LEOS_SX1262_PIN_DIO1));
    leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1262);
}

void radio_handle_dio1_irq_sx1268() {
    LOG_TRACE("radio_handle_dio1_irq_sx1268 level=%u", (unsigned)gpio_get(LEOS_SX1268_PIN_DIO1));
    leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1268);
}

void radio_service_irqs() {
    if (RADIO_900MHZ_ENABLE) {
        leos_sx126x_process_irq(LEOS_RADIO_SX1262);
    }
    if (RADIO_400MHZ_ENABLE) {
        leos_sx126x_process_irq(LEOS_RADIO_SX1268);
    }
}

int radio_send_sx1262(const uint8_t *buf, size_t len) {
    // Pretend like radio is successfully transmitting
    if (!RADIO_900MHZ_ENABLE) return 0;

    return leos_sx126x_send(LEOS_RADIO_SX1262, buf, len);
}

int radio_send_sx1268(const uint8_t *buf, size_t len) {
    // Pretend like radio is successfully transmitting
    if (!RADIO_400MHZ_ENABLE) return 0;

    return leos_sx126x_send(LEOS_RADIO_SX1268, buf, len);
}

bool radio_sx1262_packet_available() {
    if (!RADIO_900MHZ_ENABLE) return false;

    return leos_sx126x_packet_available(LEOS_RADIO_SX1262);
}

int radio_read_sx1262(
    uint8_t *buf,
    size_t buf_size,
    size_t *out_len,
    leos_radio_packet_info_t *info)
{
    if (!RADIO_900MHZ_ENABLE)
        return LEOS_RADIO_ERR_DRIVER;


    return leos_sx126x_read(
        LEOS_RADIO_SX1262,
        buf,
        buf_size,
        out_len,
        info);
}

int radio_enter_command_rx_mode() {
    if (!RADIO_900MHZ_ENABLE)
        return LEOS_RADIO_ERR_DRIVER;

    return leos_sx126x_start_rx(LEOS_RADIO_SX1262);
}

bool radio_is_sx1262_enabled() {
    return RADIO_900MHZ_ENABLE;
}

bool radio_is_sx1268_enabled() {
    return RADIO_400MHZ_ENABLE;
}
