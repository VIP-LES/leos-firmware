#include "leos/platform.hpp"

#include "leos/log.h"
#include "pico/stdlib.h"

namespace leos::platform {

bool init_log(ulog_level_t level) noexcept {
    return leos_log_init_console(level);
}

void init(ulog_level_t log_level) noexcept {
    init_log(log_level);

    // Setup indicator LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

void setup_complete() noexcept {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    LOG_INFO("Board setup complete");
}

void setup_complete(cyphal::Node& node, cyphal::Node::Health health) noexcept {
    node.setHealth(health);
    node.setMode(cyphal::Node::Mode::Operational);
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    LOG_INFO("Board setup complete (health=%u)", static_cast<unsigned>(health));
}

[[noreturn]] void fatal() noexcept {
    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(100);
    }
}

} // namespace leos::platform
