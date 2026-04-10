#pragma once
#include "leos/purpleboard.h"
#include "leos/cyphal/node.h"

#define PB_I2C_BLOCK i2c0
#define PB_PIN_SDA 8
#define PB_PIN_SCL 9
#define PB_DATA_INTERVAL_MS 1000

extern leos_purpleboard_t *pb;
extern bool pb_initialized;

void purpleboard_task();


