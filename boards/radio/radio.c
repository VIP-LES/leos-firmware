#include "radio.h"
#include "config.h"
#include "leos/log.h"

int radio_init() {
    leos_radio_hw_config_t sx1262_hw_cfg;
    leos_radio_hw_config_t sx1268_hw_cfg;
    leos_radio_config_t sx1262_cfg;
    leos_radio_config_t sx1268_cfg;

    config_build_sx1262_hw(&sx1262_hw_cfg);
    config_build_sx1268_hw(&sx1268_hw_cfg);
    config_build_sx1262(&sx1262_cfg);
    config_build_sx1268(&sx1268_cfg);

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
    leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1262);
}

void radio_handle_dio1_irq_sx1268() {
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
