#include "ads1220.hpp"

#include "pico/stdlib.h"

namespace leos::peripheral {

namespace {
bool validRegisterRange(std::uint8_t first, std::size_t count) noexcept {
    return first < 4 && count > 0 && count <= 4u - first;
}
} // namespace

void Ads1220::select() const noexcept {
    gpio_put(config_.cs_pin, false);
    sleep_us(1);
}

void Ads1220::deselect() const noexcept {
    sleep_us(1);
    gpio_put(config_.cs_pin, true);
    sleep_us(1);
}

void Ads1220::command(std::uint8_t cmd) const noexcept {
    select();
    spi_write_blocking(config_.spi, &cmd, 1);
    deselect();
}

void Ads1220::init() noexcept {
    gpio_init(config_.cs_pin);
    gpio_put(config_.cs_pin, true);
    gpio_set_dir(config_.cs_pin, GPIO_OUT);
    gpio_init(config_.drdy_pin);
    gpio_set_dir(config_.drdy_pin, GPIO_IN);
    gpio_pull_up(config_.drdy_pin);

    spi_init(config_.spi, 1000000);
    spi_set_format(config_.spi, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(config_.sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(config_.mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(config_.miso_pin, GPIO_FUNC_SPI);
    sleep_ms(1);
    reset();
}

void Ads1220::reset() noexcept {
    command(0x06);
    // At least 50 us plus 32 clock periods with the internal 4.096 MHz clock.
    sleep_ms(1);
}

void Ads1220::start() noexcept {
    command(0x08);
    // Allow DRDY to deassert before a following read polls it.
    sleep_us(60);
}

void Ads1220::powerDown() noexcept {
    command(0x02);
}

bool Ads1220::readRegisters(std::uint8_t first, std::uint8_t* values,
                            std::size_t count) noexcept {
    if (values == nullptr || !validRegisterRange(first, count)) return false;
    const std::uint8_t cmd = static_cast<std::uint8_t>(0x20u | (first << 2) | (count - 1));
    select();
    spi_write_blocking(config_.spi, &cmd, 1);
    spi_read_blocking(config_.spi, 0, values, count);
    deselect();
    return true;
}

bool Ads1220::writeRegisters(std::uint8_t first, const std::uint8_t* values,
                             std::size_t count) noexcept {
    if (values == nullptr || !validRegisterRange(first, count)) return false;
    // Register 3 bit 0 is reserved and must be zero.
    if (first + count == 4 && (values[count - 1] & 1u)) return false;
    const std::uint8_t cmd = static_cast<std::uint8_t>(0x40u | (first << 2) | (count - 1));
    select();
    spi_write_blocking(config_.spi, &cmd, 1);
    spi_write_blocking(config_.spi, values, count);
    deselect();
    sleep_us(60);
    return true;
}

bool Ads1220::dataReady() const noexcept {
    return !gpio_get(config_.drdy_pin);
}

bool Ads1220::read(std::int32_t& raw, std::uint32_t timeout_ms) noexcept {
    const absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (!dataReady()) {
        if (time_reached(deadline)) return false;
        tight_loop_contents();
    }

    std::uint8_t bytes[3];
    const std::uint8_t cmd = 0x10;
    select();
    spi_write_blocking(config_.spi, &cmd, 1);
    spi_read_blocking(config_.spi, 0, bytes, sizeof(bytes));
    deselect();

    const std::uint32_t code = (static_cast<std::uint32_t>(bytes[0]) << 16) |
                               (static_cast<std::uint32_t>(bytes[1]) << 8) | bytes[2];
    raw = static_cast<std::int32_t>(code & 0x7fffffu) -
          static_cast<std::int32_t>(code & 0x800000u);
    return true;
}

float Ads1220::toVolts(std::int32_t raw, float vref, unsigned gain) noexcept {
    return static_cast<float>(raw) * vref / (8388608.0f * static_cast<float>(gain));
}

float Ads1220::toCelsius(std::int32_t raw) noexcept {
    // Discard the ten undefined low bits, including for negative values.
    std::int32_t code = static_cast<std::int32_t>((static_cast<std::uint32_t>(raw) >> 10) & 0x3fffu);
    if (code & 0x2000) code -= 0x4000;
    return static_cast<float>(code) * 0.03125f;
}

} // namespace leos::peripheral
