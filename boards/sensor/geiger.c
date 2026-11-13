#include "hardware/clocks.h"

#include "common/platform.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/timer.h"
#include "hardware/regs/dreq.h"

#include "edge_capture.pio.h"

#include <leos/sensors/GeigerCounter_0_1.h>


#define INPUT_GPIO       14
#define BUFFER_RING_BITS 6 // 4096
#define BUFFER_SIZE      (1u << (BUFFER_RING_BITS - 2))

static uint32_t timestamp_lo[BUFFER_SIZE]
    __attribute__((aligned(16384)));

static uint32_t timestamp_hi[BUFFER_SIZE]
    __attribute__((aligned(16384)));

static uint32_t discard_word;

// DMA channels
static int dma_event;
static int dma_low;
static int dma_high;

static PIO pio = pio0;
static const uint sm = 0;

void geiger_init(void)
{
    // Configure TIMER1 to count microseconds (1 MHz).
    timer1_hw->source = 0;

    // Configure input.
    pio_gpio_init(pio, INPUT_GPIO);
    gpio_pull_down(INPUT_GPIO);

    uint offset = pio_add_program(
        pio, &edge_capture_program
    );

    pio_sm_config c =
        edge_capture_program_get_default_config(offset);

    sm_config_set_clkdiv(&c, 1.0f);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_clear_fifos(pio, sm);

    dma_event = dma_claim_unused_channel(true);
    dma_low   = dma_claim_unused_channel(true);
    dma_high  = dma_claim_unused_channel(true);

    dma_channel_config event_cfg =
        dma_channel_get_default_config(dma_event);

    channel_config_set_transfer_data_size(
        &event_cfg, DMA_SIZE_32
    );

    channel_config_set_read_increment(
        &event_cfg, false
    );

    channel_config_set_write_increment(
        &event_cfg, false
    );

    channel_config_set_dreq(
        &event_cfg,
        pio_get_dreq(pio, sm, false)
    );

    channel_config_set_chain_to(
        &event_cfg, dma_low
    );

    dma_channel_configure(
        dma_event,
        &event_cfg,
        &discard_word,
        &pio->rxf[sm],
        1,
        false
    );

    dma_channel_config low_cfg =
        dma_channel_get_default_config(dma_low);

    channel_config_set_transfer_data_size(
        &low_cfg, DMA_SIZE_32
    );

    channel_config_set_read_increment(
        &low_cfg, false
    );

    channel_config_set_write_increment(
        &low_cfg, true
    );

    channel_config_set_dreq(
        &low_cfg, DREQ_FORCE
    );

    channel_config_set_ring(
        &low_cfg, true, BUFFER_RING_BITS
    );

    channel_config_set_chain_to(
        &low_cfg, dma_high
    );

    dma_channel_configure(
        dma_low,
        &low_cfg,
        timestamp_lo,
        &timer1_hw->timelr,
        1,
        false
    );

    dma_channel_config high_cfg =
        dma_channel_get_default_config(dma_high);

    channel_config_set_transfer_data_size(
        &high_cfg, DMA_SIZE_32
    );

    channel_config_set_read_increment(
        &high_cfg, false
    );

    channel_config_set_write_increment(
        &high_cfg, true
    );

    channel_config_set_dreq(
        &high_cfg, DREQ_FORCE
    );

    channel_config_set_ring(
        &high_cfg, true, BUFFER_RING_BITS
    );

    channel_config_set_chain_to(
        &high_cfg, dma_event
    );

    dma_channel_configure(
        dma_high,
        &high_cfg,
        timestamp_hi,
        &timer1_hw->timehr,
        1,
        false
    );

    dma_start_channel_mask(1u << dma_event);
    pio_sm_set_enabled(pio, sm, true);
}

#define GEIGER_BATCH_SIZE (BUFFER_SIZE / 2)
#define GEIGER_TIMEOUT_MS 500

void geiger_task(void) {
    if (&leos_node == NULL) {
        return;
    }

    static bool initialized = false;
    static uint32_t read_index = 0;
    static uint32_t last_write_index = 0;
    static uint32_t pending_count = 0;
    static absolute_time_t next_run = 0;

    const uint32_t buffer_size = BUFFER_SIZE;

    // Get current DMA write position.
    uintptr_t addr =
        (uintptr_t)dma_channel_hw_addr(dma_high)->write_addr;

    uintptr_t base = (uintptr_t)&timestamp_hi[0];

    uint32_t write_index =
        (uint32_t)((addr - base) / sizeof(timestamp_hi[0]));

    // Initialize tracking on first invocation.
    if (!initialized) {
        read_index = write_index;
        last_write_index = write_index;
        next_run = make_timeout_time_ms(GEIGER_TIMEOUT_MS);
        initialized = true;
        return;
    }

    // Calculate newly written samples, accounting for DMA wrapping.
    uint32_t newly_written =
        (write_index + buffer_size - last_write_index) % buffer_size;

    last_write_index = write_index;
    pending_count += newly_written;

    absolute_time_t now = get_absolute_time();

    // Determine whether we should transmit.
    bool batch_full = pending_count >= GEIGER_BATCH_SIZE;

    bool timeout =
        absolute_time_diff_us(now, next_run) <= 0;

    if (pending_count == 0 || (!batch_full && !timeout)) {
        return;
    }

    LOG_INFO("Start Geiger burst: %u samples\n",
             (unsigned)pending_count);

    leos_sensors_GeigerCounter_0_1 message;

    message.counts.count = pending_count;

    for (uint32_t i = 0; i < pending_count; i++) {
        uint32_t index = (read_index + i) % buffer_size;

        uint32_t hi = timestamp_hi[index];
        uint32_t lo = timestamp_lo[index];

        uint64_t timestamp_us =
            (((uint64_t)hi << 32) | lo);

        message.counts.elements[i] = timestamp_us;
    }

    // Advance the read position.
    read_index = (read_index + pending_count) % buffer_size;
    pending_count = 0;

    uint8_t buffer[
        leos_sensors_GeigerCounter_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_
    ];

    size_t size = sizeof(buffer);

    int8_t result =
        leos_sensors_GeigerCounter_0_1_serialize_(
            &message, buffer, &size
        );

    LOG_INFO("Serialization result: %d, size: %u\n",
             result, (unsigned)size);

    static CanardTransferID transfer_id = 0;

    const struct CanardTransferMetadata md = {
        .port_id = leos_sensors_GeigerCounter_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .remote_node_id = CANARD_NODE_ID_UNSET,
        .transfer_id = transfer_id++,
        .transfer_kind = CanardTransferKindMessage
    };

    const struct CanardPayload payload = {
        .data = buffer,
        .size = size
    };

    if (leos_cyphal_push(&leos_node, &md, payload)
            != LEOS_CYPHAL_OK) {
        LOG_DEBUG("Couldn't push GeigerCounter message");
    } else {
        LOG_DEBUG("Pushed GeigerCounter OK");
    }

    leos_net_task();

    // Start a new 500ms timeout.
    next_run = make_timeout_time_ms(GEIGER_TIMEOUT_MS);

    LOG_INFO("End Geiger burst\n");
}
