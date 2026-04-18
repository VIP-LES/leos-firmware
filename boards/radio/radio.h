#pragma once
#include <stdint.h>
#include "leos/sx126x.h"


int radio_init();

void radio_poll();

int radio_start_send_sx1262(const uint8_t *buf, size_t len);
int radio_start_send_sx1268(const uint8_t *buf, size_t len);

void radio_handle_dio1_irq_sx1262();
void radio_handle_dio1_irq_sx1268();

bool radio_sx1262_packet_available();

int radio_read_sx1262(
    uint8_t *buf,
    size_t buf_size,
    size_t *out_len,
    leos_radio_packet_info_t *info);

int radio_enter_command_rx_mode();

bool radio_is_sx1262_enabled();
bool radio_is_sx1268_enabled();
