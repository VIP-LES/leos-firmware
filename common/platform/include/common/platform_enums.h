#pragma once

typedef enum {
    BOARD_HEALTH_NOMINAL = 0U,
    BOARD_HEALTH_ADVISORY = 1U,
    BOARD_HEALTH_CAUTION = 2U,
    BOARD_HEALTH_WARNING = 3U
} board_health_t;