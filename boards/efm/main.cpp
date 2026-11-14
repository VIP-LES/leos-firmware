#include "ads1220.hpp"
#include "leos/firmware.hpp"

#include <leos/efm/EFM_0_2.h>

#include "pico/multicore.h"
#include "pico/stdio_usb.h"
#include "pico/util/queue.h"

#include <atomic>
#include <cstdio>

using namespace leos;

namespace {
constexpr std::uint8_t EFM_NODE_ID = 12;
constexpr std::size_t SAMPLE_QUEUE_CAPACITY = 1024;
constexpr std::size_t BATCH_CAPACITY = leos_efm_SampleBatch_0_2_readings_ARRAY_CAPACITY_;
constexpr std::uint32_t BATCH_LATENCY_US = 8000;

peripheral::Ads1220 adc{{spi1, 10, 11, 12, 13, 19}};
cyphal::Node node{can_hw_config};
cyphal::Publisher efm_publisher{node, leos_efm_EFM_0_2_FIXED_PORT_ID_};
queue_t sample_queue;
std::atomic<bool> adc_fault{false};
std::atomic<std::uint32_t> samples_acquired{0};
std::atomic<std::uint32_t> samples_queue_dropped{0};
std::atomic<std::uint32_t> adc_read_timeouts{0};

bool publish_samples(const leos_efm_SampleBatch_0_2& samples) {
    leos_efm_EFM_0_2 message{};
    leos_efm_EFM_0_2_select_samples_(&message);
    message.samples = samples;

    std::uint8_t buffer[leos_efm_EFM_0_2_SERIALIZATION_BUFFER_SIZE_BYTES_];
    std::size_t size = sizeof(buffer);
    if (leos_efm_EFM_0_2_serialize_(&message, buffer, &size) != 0) {
        LOG_WARNING("Could not serialize EFM sample batch");
        return false;
    }
    if (efm_publisher.publish(buffer, size) != cyphal::Publisher::Result::Queued) {
        LOG_WARNING("Could not queue EFM sample batch");
        return false;
    }
    return true;
}

void acquire() {
    adc.init();
    const std::uint8_t config[4] = {
        // EFM0 = AIN0 - AIN1, gain 2, full scale +/-1.024 V.
        static_cast<std::uint8_t>(peripheral::ads1220::reg0::MUX_AIN0_AIN1 |
                                  peripheral::ads1220::reg0::GAIN_2),
        static_cast<std::uint8_t>(peripheral::ads1220::reg1::RATE_1000 |
                                  peripheral::ads1220::reg1::MODE_TURBO |
                                  peripheral::ads1220::reg1::CONTINUOUS),
        peripheral::ads1220::reg2::REF_INTERNAL,
        0,
    };
    std::uint8_t readback[4]{};
    if (!adc.writeRegisters(0, config, 4) ||
        !adc.readRegisters(0, readback, 4)) {
        adc_fault.store(true, std::memory_order_relaxed);
        return;
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (readback[i] != config[i]) {
            adc_fault.store(true, std::memory_order_relaxed);
            return;
        }
    }

    gpio_init(16);
    gpio_set_dir(16, GPIO_IN);
    gpio_init(17);
    gpio_set_dir(17, GPIO_IN);
    adc.start();

    std::uint32_t sequence = 0;
    std::uint8_t channel = 0;
    while (true) {
        leos_efm_Sample_0_1 sample{};
        if (!adc.read(sample.raw, 100)) {
            adc_read_timeouts.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        sample.timestamp_us = time_us_64();
        sample.sequence = sequence++;
        sample.channel = channel;
        sample.hall = static_cast<std::uint8_t>((gpio_get(16) ? 1u : 0u) |
                                                (gpio_get(17) ? 2u : 0u));
        samples_acquired.fetch_add(1, std::memory_order_relaxed);
        // Never pause acquisition for CAN. Gaps in sequence reveal dropped samples.
        if (!queue_try_add(&sample_queue, &sample)) {
            samples_queue_dropped.fetch_add(1, std::memory_order_relaxed);
        }

        const std::uint8_t next_channel = static_cast<std::uint8_t>(1u - channel);
        const std::uint8_t reg0 = next_channel
            ? static_cast<std::uint8_t>(peripheral::ads1220::reg0::MUX_AIN2_AIN3 |
                                        peripheral::ads1220::reg0::GAIN_1)
            : static_cast<std::uint8_t>(peripheral::ads1220::reg0::MUX_AIN0_AIN1 |
                                        peripheral::ads1220::reg0::GAIN_2);
        // Changing the mux restarts conversion for the newly selected pair.
        if (!adc.writeRegisters(0, &reg0, 1)) {
            adc_fault.store(true, std::memory_order_relaxed);
            return;
        }
        channel = next_channel;
    }
}
} // namespace

int main() {
    platform::init();
    // Calibration controls are disabled; hold their outputs at zero.
    constexpr uint calibration_pins[] = {21u, 22u, 26u};
    for (const uint pin : calibration_pins) {
        gpio_init(pin);
        gpio_put(pin, false);
        gpio_set_dir(pin, GPIO_OUT);
    }
    if (!queue_init(&sample_queue, sizeof(leos_efm_Sample_0_1),
                    SAMPLE_QUEUE_CAPACITY)) {
        LOG_ERROR("Could not allocate EFM sample queue");
        platform::fatal();
    }

    const eERRORRESULT result = node.init(can_config, EFM_NODE_ID);
    if (result == ERR_OK) {
        platform::setup_complete(node, Health::Nominal);
    } else {
        LOG_ERROR("CAN bus initialization failed: %d", result);
        platform::setup_complete();
    }

    multicore_launch_core1(acquire);

    leos_efm_SampleBatch_0_2 batch{};
    absolute_time_t batch_deadline{};
    absolute_time_t next_stats = make_timeout_time_ms(1000);
    std::uint32_t previous_acquired = 0;
    std::uint32_t previous_queued = 0;
    std::uint32_t samples_queued = 0;
    std::uint32_t samples_publish_rejected = 0;
    bool fault_reported = false;
    while (true) {
        node.poll();
        if (!fault_reported && adc_fault.load(std::memory_order_relaxed)) {
            LOG_ERROR("ADC configuration failed; check wiring");
            if (result == ERR_OK) node.setHealth(Health::Caution);
            fault_reported = true;
        }

        while (batch.readings.count < BATCH_CAPACITY &&
               queue_try_remove(&sample_queue,
                                &batch.readings.elements[batch.readings.count])) {
            if (batch.readings.count == 0) {
                batch_deadline = make_timeout_time_us(BATCH_LATENCY_US);
            }
            ++batch.readings.count;
        }

        if (batch.readings.count != 0 &&
            (batch.readings.count == BATCH_CAPACITY || time_reached(batch_deadline))) {
            if (publish_samples(batch)) {
                samples_queued += batch.readings.count;
            } else {
                samples_publish_rejected += batch.readings.count;
            }
            batch.readings.count = 0;
        }
        if (time_reached(next_stats)) {
            const std::uint32_t acquired = samples_acquired.load(std::memory_order_relaxed);
            if (stdio_usb_connected()) {
                int tx_error_count = -1;
                int can_error_status = -1;
                int txq_status = -1;
                if (result == ERR_OK) {
                    std::uint8_t tec = 0;
                    eMCP251XFD_TXRXErrorStatus error_status{};
                    if (MCP251XFD_GetTransmitReceiveErrorCountAndStatus(
                            &node.mcp().native, &tec, nullptr, &error_status) == ERR_OK) {
                        tx_error_count = tec;
                        can_error_status = static_cast<int>(error_status);
                    }
                    setMCP251XFD_TXQstatus txq{};
                    if (MCP251XFD_GetTXQStatus(&node.mcp().native, &txq) == ERR_OK) {
                        txq_status = static_cast<int>(txq);
                    }
                }
                std::printf("# EFM acq/s=%lu queued/s=%lu qdrop=%lu reject=%lu "
                            "timeout=%lu tx_expired=%llu tx_sacrificed=%llu q=%u "
                            "tec=%d canerr=0x%02x txq=0x%02x\n",
                            static_cast<unsigned long>(acquired - previous_acquired),
                            static_cast<unsigned long>(samples_queued - previous_queued),
                            static_cast<unsigned long>(samples_queue_dropped.load(std::memory_order_relaxed)),
                            static_cast<unsigned long>(samples_publish_rejected),
                            static_cast<unsigned long>(adc_read_timeouts.load(std::memory_order_relaxed)),
                            static_cast<unsigned long long>(node.canard().err.tx_expiration),
                            static_cast<unsigned long long>(node.canard().err.tx_sacrifice),
                            queue_get_level(&sample_queue), tx_error_count,
                            can_error_status, txq_status);
            }
            previous_acquired = acquired;
            previous_queued = samples_queued;
            next_stats = make_timeout_time_ms(1000);
        }
        tight_loop_contents();
    }
}
