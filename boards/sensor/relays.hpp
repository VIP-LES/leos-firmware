#pragma once

#include "leos/cyphal/service.hpp"

#include <cstddef>
#include <cstdint>
#include "pico/time.h"

class Relays final {
public:
    explicit Relays(leos::cyphal::Node& node) noexcept;

    // Configure all outputs off before starting CAN or accepting requests.
    void initHardware() noexcept;

    // Register the relay service after Node::init().
    bool serve() noexcept;

    // Call after Node::poll() to send replies and expire timed relay outputs.
    void poll() noexcept;

private:
    struct RelayState {
        absolute_time_t turn_off_at{};
        bool enabled = false;
        bool has_deadline = false;
    };

    static void onRequest(leos::cyphal::Service& service,
                          const leos::cyphal::Request& request,
                          void* context) noexcept;
    void handleRequest(leos::cyphal::Service& service,
                       const leos::cyphal::Request& request) noexcept;
    void setRelay(std::size_t index, bool enabled, std::int32_t duration_ms) noexcept;

    static constexpr std::uint8_t relay_pins_[4] = {18, 19, 20, 21};
    leos::cyphal::Service service_;
    RelayState relay_state_[4]{};
    bool hardware_initialized_ = false;
};
