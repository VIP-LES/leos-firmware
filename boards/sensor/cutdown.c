#include "common/platform.h"
#include "cutdown.h"
#include "pico/stdlib.h"
#include <leos/service/Cutdown_0_1.h>

static bool start_cutdown = false;

// State for non-blocking sequence
static bool cutdown_running = false;
static absolute_time_t cutdown_timeout = 0;
#define CUTDOWN_DURATION_MS 3000ul

void onCutdownRequest(struct CanardRxTransfer *transfer, void* ref) {
    (void) ref;
    start_cutdown = true;

    static CanardTransferID tid = 0;
    leos_service_Cutdown_Response_0_1 res = {
        .status = leos_service_Cutdown_Response_0_1_STATUS_IN_PROGRESS
    };
    uint8_t buffer[leos_service_Cutdown_Response_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = leos_service_Cutdown_Response_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_;
    if (leos_service_Cutdown_Response_0_1_serialize_(&res, buffer, &size) < 0) {
        LOG_DEBUG("Failed to serialize cutdown response");
    }
    const struct CanardTransferMetadata md = {
        .port_id = leos_service_Cutdown_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .transfer_id = tid++,
        .transfer_kind = CanardTransferKindResponse,
        .remote_node_id = transfer->metadata.remote_node_id
    };
    const struct CanardPayload p = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(&leos_node, &md, p) != LEOS_CYPHAL_OK) {
        LOG_DEBUG("Failed to push cutdown response");
    }
}

void cutdown_init() {
    if (!&leos_node) return;
    gpio_init(R1);
    gpio_init(R2);
    gpio_init(R3);
    gpio_init(R4);
    gpio_set_dir(R1, GPIO_OUT);
    gpio_set_dir(R2, GPIO_OUT);
    gpio_set_dir(R3, GPIO_OUT);
    gpio_set_dir(R4, GPIO_OUT);

    LOG_DEBUG("Subscribing to cutdown requests");
    leos_cyphal_subscribe(
        &leos_node, 
        CanardTransferKindResponse, 
        leos_service_Cutdown_0_1_FIXED_PORT_ID_, 
        leos_service_Cutdown_Request_0_1_EXTENT_BYTES_, 
        onCutdownRequest, 
        &leos_node
    );
}

void cutdown_start() {
    start_cutdown = true;
}

// For gunpowder cutdown, we close all relays for CUTDOWN_DURATION_MS
void cutdown_task() {
    // If a request arrived, start the non-blocking sequence if not already running
    if (start_cutdown && !cutdown_running) {
        start_cutdown = false; // consume request
        cutdown_running = true;

        // Turn on relays
        gpio_put(R1, 1);
        gpio_put(R2, 1);
        gpio_put(R3, 1);
        gpio_put(R4, 1);

        // set timeout to disable relays
        cutdown_timeout = make_timeout_time_ms(CUTDOWN_DURATION_MS);
    }

    if (!cutdown_running) return;

    absolute_time_t now = get_absolute_time();

    // If current index duration elapsed, move to next
    if (now > cutdown_timeout) {

        // Turn off relays
        gpio_put(R1, 0);
        gpio_put(R2, 0);
        gpio_put(R3, 0);
        gpio_put(R4, 0);

        // finished
        cutdown_running = false;
        LOG_DEBUG("Cutdown sequence complete");
    }
}