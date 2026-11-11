#include "relays.hpp"

#include <leos/relays/Relays_1_0.h>

#include "hardware/gpio.h"
#include "leos/log.h"
#include "pico/time.h"

constexpr std::uint16_t RELAYS_SERVICE_ID = leos_relays_Relays_1_0_FIXED_PORT_ID_;
constexpr std::size_t RELAYS_REQUEST_EXTENT = leos_relays_Relays_Request_1_0_EXTENT_BYTES_;
constexpr std::uint8_t Relays::relay_pins_[4];

Relays::Relays(leos::cyphal::Node& node) noexcept
    : service_(node, RELAYS_SERVICE_ID, RELAYS_REQUEST_EXTENT,
               &Relays::onRequest, this) {}

void Relays::initHardware() noexcept {
    if (hardware_initialized_) {
        return;
    }

    for (std::size_t index = 0; index < 4; ++index) {
        gpio_init(relay_pins_[index]);
        gpio_put(relay_pins_[index], false);
        gpio_set_dir(relay_pins_[index], GPIO_OUT);
    }
    hardware_initialized_ = true;
    LOG_INFO("Relay outputs initialized off");
}

bool Relays::serve() noexcept {
    if (!hardware_initialized_) {
        LOG_ERROR("Cannot serve relay requests before relay GPIO initialization");
        return false;
    }
    return service_.serve();
}

void Relays::poll() noexcept {
    const absolute_time_t now = get_absolute_time();
    for (std::size_t index = 0; index < 4; ++index) {
        auto& state = relay_state_[index];
        if (state.enabled && state.has_deadline && time_reached(state.turn_off_at)) {
            gpio_put(relay_pins_[index], false);
            state = {};
            LOG_INFO("Relay %u timed out and turned off", static_cast<unsigned>(index + 1));
        }
    }
    service_.poll();
}

void Relays::onRequest(leos::cyphal::Service& service,
                       const leos::cyphal::Request& request,
                       void* context) noexcept {
    static_cast<Relays*>(context)->handleRequest(service, request);
}

void Relays::handleRequest(leos::cyphal::Service& service,
                           const leos::cyphal::Request& request) noexcept {
    leos_relays_Relays_Request_1_0 command{};
    std::size_t payload_size = request.payload.size;
    std::uint8_t status = leos_relays_Relays_Response_1_0_FAILED;

    if ((leos_relays_Relays_Request_1_0_deserialize_(
             &command, static_cast<const std::uint8_t*>(request.payload.data),
             &payload_size) < 0) ||
        (payload_size != request.payload.size)) {
        LOG_WARNING("Invalid relay request payload");
    } else if (command.relay_index > 4U) {
        status = leos_relays_Relays_Response_1_0_INVALID_INDEX;
        LOG_WARNING("Relay request has invalid index %u", (unsigned)command.relay_index);
    } else if (command.enabled && (command.auto_off_after_ms == 0 ||
                                   command.auto_off_after_ms < -1)) {
        status = leos_relays_Relays_Response_1_0_INVALID_DURATION;
        LOG_WARNING("Relay request has invalid duration %ld",
                    static_cast<long>(command.auto_off_after_ms));
    } else {
        const std::size_t first = command.relay_index == 0U
            ? 0U : static_cast<std::size_t>(command.relay_index - 1U);
        const std::size_t limit = command.relay_index == 0U ? 4U : first + 1U;
        for (std::size_t index = first; index < limit; ++index) {
            setRelay(index, command.enabled, command.auto_off_after_ms);
        }
        status = leos_relays_Relays_Response_1_0_OK;
        LOG_INFO("Relay request applied index=%u enabled=%u duration_ms=%ld",
                 (unsigned)command.relay_index, command.enabled ? 1U : 0U,
                 static_cast<long>(command.auto_off_after_ms));
    }

    if (!service.reply(request, &status, sizeof(status))) {
        LOG_ERROR("Relay service response could not be queued");
    }
}

void Relays::setRelay(std::size_t index, bool enabled, std::int32_t duration_ms) noexcept {
    auto& state = relay_state_[index];
    gpio_put(relay_pins_[index], enabled);
    state.enabled = enabled;
    state.has_deadline = enabled && (duration_ms > 0);
    state.turn_off_at = state.has_deadline
        ? make_timeout_time_ms(static_cast<std::uint32_t>(duration_ms))
        : absolute_time_t{};
}
