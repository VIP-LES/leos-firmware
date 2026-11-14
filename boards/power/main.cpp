#include "leos/firmware.hpp"
#include "leos/peripheral/mcp41100.hpp"
#include <leos/power/VariableVoltage_0_1.h>

using namespace leos;

// Approximate adjustable-output voltages to wiper codes. 
// This is estimated using a 6S LiPo.
enum Voltage : std::uint8_t {
    V17_5 = 0, V13_95 = 1, V11_67 = 2, V10_02 = 3,
    V7_89 = 5, V6_09 = 8, V5_34 = 10, V5_04 = 11,
    V4_78 = 12, V4_17 = 15, V3_50 = 20, V2_67 = 32,
    V1_89 = 64, V1_47 = 128, V1_33 = 192, V1_25 = 255,
};

cyphal::Node node{can_hw_config};
peripheral::Mcp41100 adj_output{{spi1, 1000000u, 10u, 11u, 13u}};

static void on_request(cyphal::Service& service, const cyphal::Request& transfer, void*) {
    std::uint8_t status = leos_power_VariableVoltage_Response_0_1_ERR;
    LOG_INFO("voltage request received: size=%zu", transfer.payload.size);
    if (transfer.payload.size == 1U) {
        const auto code = *static_cast<const std::uint8_t*>(transfer.payload.data);
        if (adj_output.setWiper(code)) {
            status = leos_power_VariableVoltage_Response_0_1_OK;
            LOG_INFO("voltage wiper code %u", (unsigned)code);
        }
    }
    if (status != leos_power_VariableVoltage_Response_0_1_OK)
        LOG_WARNING("voltage request failed");
    if (!service.reply(transfer, &status, sizeof(status)))
        LOG_ERROR("voltage response could not be queued");
}

cyphal::Service voltage_service{
    node,
    leos_power_VariableVoltage_0_1_FIXED_PORT_ID_,
    leos_power_VariableVoltage_Request_0_1_EXTENT_BYTES_,
    on_request
};

int main() {
    platform::init(ULOG_DEBUG_LEVEL);

    // Initialize adjustable output potentiometer and set default voltage
    adj_output.init();
    sleep_ms(3000);
    if (adj_output.setWiper(Voltage::V5_04)) {
        LOG_INFO("Set adjustable output to default 5V.");
    }

    const eERRORRESULT result = node.init(can_config, LEOS_POWER_NODE_ID);
    if (result == ERR_OK) {
        if (voltage_service.serve()) 
            platform::setup_complete(node, Health::Nominal);
        else {
            LOG_ERROR("voltage service subscription failed");
            platform::setup_complete(node, Health::Caution);
        }
    } else {
        LOG_ERROR("CANBus init failed: %d", result);
        // The power module stays active even without CAN communication.
        platform::setup_complete();
    }

    int x = 0;

    while (true) {
        node.poll();
        voltage_service.poll();
    }
}
