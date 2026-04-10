
#include "leos/mcp342x.h"
#include "leos/efm/ADC_0_1.h"
#include "leos/cyphal/node.h"
#include "common/platform.h"

#define LOCATION_NAME "the edge of space"

static void set_cyphal_string(uavcan_primitive_String_1_0* string, const char* value) {
    string->value.count = strlen(value);
    memcpy(string->value.elements, value, string->value.count);
}

void efm_task(mcp342x_dev_t *adc1, mcp342x_dev_t *adc2) {
    int32_t val1, val2;

    leos_efm_ADC_0_1 message;

    for (int ch = 0; ch < 4; ch++) {
        mcp342x_channel_t channel = (mcp342x_channel_t)ch;

        float voltage1;
        float voltage2;

        // --- ADC 1 ---
        mcp342x_set_channel(adc1, channel);

        if (mcp342x_read_voltage(adc1, &voltage1) < 0) {
            LOG_WARNING("Failed to read ADC1 voltage.");
        }

        // --- ADC 2 ---
        mcp342x_set_channel(adc2, channel);

        if (mcp342x_read_voltage(adc2, &voltage2) < 0){
            LOG_WARNING("Failed to read ADC2 voltage.");
        }

        LOG_TRACE("CH%d: ADC1=%.5f ADC2=%.5f", ch, voltage1, voltage2);

        switch(ch) {
            case 0:
                message.adc1_ch1_diff = voltage1;
                message.adc2_ch1_diff = voltage2;
                break;
            case 1:
                message.adc1_ch2_sensing = voltage1;
                message.adc2_ch2_sensing = voltage2;
                break;
            case 2:
                message.adc1_ch3_reference = voltage1;
                message.adc2_ch3_reference = voltage2;
                break;
            case 3:
                message.adc1_ch4_breakbeam = voltage1;
                message.adc2_ch4_breakbeam = voltage2;
        }
    }

    set_cyphal_string(&message.location, LOCATION_NAME);
    message.valid = true;

    // Send cyphal message with these collected ADC values
    static CanardTransferID transfer_id = 0;
    uint8_t buffer[leos_efm_ADC_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_efm_ADC_0_1_serialize_(&message, buffer, &size) != 0) {
        LOG_WARNING("Failed to serialize EFM ADC message");
        return;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_efm_ADC_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityLow,
        .remote_node_id = CANARD_NODE_ID_UNSET,
        .transfer_id = transfer_id++,
        .transfer_kind = CanardTransferKindMessage
    };
    const struct CanardPayload payload = {
        .data = buffer,
        .size = size
    };

    if (leos_cyphal_push(&leos_node, &md, payload) != LEOS_CYPHAL_OK) {
        LOG_WARNING("Failed to push EFM ADC message.");
    }

    LOG_DEBUG("Queued EFM ADC Cyphal message.");
}