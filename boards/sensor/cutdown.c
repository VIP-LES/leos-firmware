#include "common/platform.h"
#include "cutdown.h"
#include "pico/stdlib.h"
#include <leos/cutdown/Cutdown_0_2.h>
#include <leos/cutdown/CutdownStatus_0_1.h>

static bool start_cutdown = false;

// State for non-blocking sequence
static bool cutdown_running = false;
static absolute_time_t cutdown_timeout = 0;
#define CUTDOWN_DURATION_MS 3000ul

int send_response(uint8_t status, struct CanardRxTransfer *transfer) {
    leos_cutdown_Cutdown_Response_0_2 response = {
        .status = status
    };
    uint8_t buffer[leos_cutdown_Cutdown_Response_0_2_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_cutdown_Cutdown_Response_0_2_serialize_(&response, buffer, &size) < 0) {
        LOG_WARNING("Failed to serialize cutdown response");
        return -1;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_cutdown_Cutdown_0_2_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .transfer_id = transfer->metadata.transfer_id,
        .transfer_kind = CanardTransferKindResponse,
        .remote_node_id = transfer->metadata.remote_node_id
    };
    const struct CanardPayload p = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(&leos_node, &md, p) != LEOS_CYPHAL_OK) {
        LOG_WARNING("Failed to push cutdown response");
        return -1;
    }
    return 0;
}

int publish_status(uint8_t status) {
    static CanardTransferID tid = 0;

    leos_cutdown_CutdownStatus_0_1 msg = {
        .status = status
    };

    uint8_t buffer[leos_cutdown_CutdownStatus_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_cutdown_CutdownStatus_0_1_serialize_(&msg, buffer, &size) < 0) {
        LOG_WARNING("Failed to serialize cutdown status message");
        return -1;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_cutdown_CutdownStatus_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .transfer_id = tid++,
        .transfer_kind = CanardTransferKindMessage,
        .remote_node_id = CANARD_NODE_ID_UNSET
    };
    const struct CanardPayload p = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(&leos_node, &md, p) != LEOS_CYPHAL_OK) {
        LOG_WARNING("Failed to push cutdown status message");
        return -1;
    }
    return 0;
}

void onCutdownRequest(struct CanardRxTransfer *transfer, void* ref) {
    (void) ref;

    uint8_t status;
    if (cutdown_running || start_cutdown) {
        status = leos_cutdown_Cutdown_Response_0_2_STATUS_ALREADY_IN_PROGRESS;
    } else {
        status = leos_cutdown_Cutdown_Response_0_2_STATUS_ACCEPTED;
        start_cutdown = true;
    }
    send_response(status, transfer);
}

void cutdown_init() {
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
        CanardTransferKindRequest, 
        leos_cutdown_Cutdown_0_2_FIXED_PORT_ID_, 
        leos_cutdown_Cutdown_Request_0_2_EXTENT_BYTES_, 
        onCutdownRequest, 
        NULL
    );
    
    // Send idle message at startup
    publish_status(leos_cutdown_CutdownStatus_0_1_STATUS_IDLE);
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

        publish_status(leos_cutdown_CutdownStatus_0_1_STATUS_IN_PROGRESS);
    }

    if (!cutdown_running) return;

    absolute_time_t now = get_absolute_time();

    // If current index duration elapsed, move to next
    if (absolute_time_diff_us(now, cutdown_timeout) <= 0) {

        // Turn off relays
        gpio_put(R1, 0);
        gpio_put(R2, 0);
        gpio_put(R3, 0);
        gpio_put(R4, 0);

        // finished
        cutdown_running = false;

        // Send FINISHED and IDLE messages
        publish_status(leos_cutdown_CutdownStatus_0_1_STATUS_FINISHED);
        publish_status(leos_cutdown_CutdownStatus_0_1_STATUS_IDLE);

        LOG_DEBUG("Cutdown sequence complete");
    }
}