#pragma once

#include <cstddef>
#include <cstdint>

#include "hardware/spi.h"

namespace leos::peripheral {

// Combine register fields with bitwise OR before writing them to the ADC.
namespace ads1220 {
namespace reg0 {
constexpr std::uint8_t MUX_AIN0_AIN1 = 0u << 4;
constexpr std::uint8_t MUX_AIN0_AIN2 = 1u << 4;
constexpr std::uint8_t MUX_AIN0_AIN3 = 2u << 4;
constexpr std::uint8_t MUX_AIN1_AIN2 = 3u << 4;
constexpr std::uint8_t MUX_AIN1_AIN3 = 4u << 4;
constexpr std::uint8_t MUX_AIN2_AIN3 = 5u << 4;
constexpr std::uint8_t MUX_AIN1_AIN0 = 6u << 4;
constexpr std::uint8_t MUX_AIN3_AIN2 = 7u << 4;
constexpr std::uint8_t MUX_AIN0_AVSS = 8u << 4;
constexpr std::uint8_t MUX_AIN1_AVSS = 9u << 4;
constexpr std::uint8_t MUX_AIN2_AVSS = 10u << 4;
constexpr std::uint8_t MUX_AIN3_AVSS = 11u << 4;
constexpr std::uint8_t MUX_REFERENCE_DIV4 = 12u << 4;
constexpr std::uint8_t MUX_SUPPLY_DIV4 = 13u << 4;
constexpr std::uint8_t MUX_SHORTED = 14u << 4;
constexpr std::uint8_t GAIN_1 = 0u << 1;
constexpr std::uint8_t GAIN_2 = 1u << 1;
constexpr std::uint8_t GAIN_4 = 2u << 1;
constexpr std::uint8_t GAIN_8 = 3u << 1;
constexpr std::uint8_t GAIN_16 = 4u << 1;
constexpr std::uint8_t GAIN_32 = 5u << 1;
constexpr std::uint8_t GAIN_64 = 6u << 1;
constexpr std::uint8_t GAIN_128 = 7u << 1;
constexpr std::uint8_t PGA_BYPASS = 0x01u;
} // namespace reg0

namespace reg1 {
// Rates are for normal mode with the internal clock.
constexpr std::uint8_t RATE_20 = 0u << 5;
constexpr std::uint8_t RATE_45 = 1u << 5;
constexpr std::uint8_t RATE_90 = 2u << 5;
constexpr std::uint8_t RATE_175 = 3u << 5;
constexpr std::uint8_t RATE_330 = 4u << 5;
constexpr std::uint8_t RATE_600 = 5u << 5;
constexpr std::uint8_t RATE_1000 = 6u << 5;
constexpr std::uint8_t MODE_NORMAL = 0u << 3;
constexpr std::uint8_t MODE_DUTY_CYCLE = 1u << 3;
constexpr std::uint8_t MODE_TURBO = 2u << 3;
constexpr std::uint8_t CONTINUOUS = 0x04u;
constexpr std::uint8_t TEMPERATURE = 0x02u;
constexpr std::uint8_t BURNOUT = 0x01u;
} // namespace reg1

namespace reg2 {
constexpr std::uint8_t REF_INTERNAL = 0u << 6;
constexpr std::uint8_t REF_EXTERNAL0 = 1u << 6;
constexpr std::uint8_t REF_EXTERNAL1 = 2u << 6;
constexpr std::uint8_t REF_SUPPLY = 3u << 6;
constexpr std::uint8_t FILTER_50_60HZ = 1u << 4;
constexpr std::uint8_t FILTER_50HZ = 2u << 4;
constexpr std::uint8_t FILTER_60HZ = 3u << 4;
constexpr std::uint8_t LOW_SIDE_SWITCH = 0x08u;
// IDAC current occupies bits 2:0.
} // namespace reg2
} // namespace ads1220

class Ads1220 {
public:
    struct Config {
        spi_inst_t* spi;
        uint sck_pin;
        uint mosi_pin;
        uint miso_pin;
        uint cs_pin;
        uint drdy_pin; // Dedicated active-low DRDY, not DOUT/DRDY.
    };

    explicit Ads1220(Config config) noexcept : config_(config) {}

    // Supply must be stable. Configures SPI for 1 MHz, mode 1, then resets.
    // GPIOs must be valid and distinct. Calls block and must be serialized with
    // other SPI users. Restore mode 1 / 1 MHz if another device changes the bus.
    void init() noexcept;
    void reset() noexcept;
    void start() noexcept;
    void powerDown() noexcept;
    bool readRegisters(std::uint8_t first, std::uint8_t* values, std::size_t count) noexcept;
    bool writeRegisters(std::uint8_t first, const std::uint8_t* values, std::size_t count) noexcept;
    bool dataReady() const noexcept;

    // Does not start a conversion. On timeout, raw remains unchanged.
    // timeout_ms = 0 polls once without waiting.
    bool read(std::int32_t& raw, std::uint32_t timeout_ms) noexcept;

    // vref is the actual reference voltage; gain is 1, 2, 4, ... 128.
    static float toVolts(std::int32_t raw, float vref, unsigned gain) noexcept;
    // Only valid when the TEMPERATURE bit is set in register 1.
    static float toCelsius(std::int32_t raw) noexcept;

private:
    void select() const noexcept;
    void deselect() const noexcept;
    void command(std::uint8_t cmd) const noexcept;

    Config config_;
};

} // namespace leos::peripheral
