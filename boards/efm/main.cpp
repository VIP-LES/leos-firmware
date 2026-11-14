#include "leos/firmware.hpp"
#include <leos/efm/EFM_0_1.h>

using namespace leos;

constexpr std::uint8_t EFM_NODE_ID = 12;

cyphal::Node node{can_hw_config};
cyphal::Publisher efm_publisher{node, leos_efm_EFM_0_1_FIXED_PORT_ID_};

// Populate one union variant and select its tag before calling this helper.
bool publish_efm(const leos_efm_EFM_0_1& message) {
    std::uint8_t buffer[leos_efm_EFM_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    std::size_t size = sizeof(buffer);
    if (leos_efm_EFM_0_1_serialize_(&message, buffer, &size) != 0) {
        LOG_WARNING("Could not serialize EFM message");
        return false;
    }
    if (efm_publisher.publish(buffer, size) != cyphal::Publisher::Result::Queued) {
        LOG_WARNING("Could not queue EFM message");
        return false;
    }
    return true;
}

int main() {
    platform::init();

    const eERRORRESULT result = node.init(can_config, EFM_NODE_ID);
    if (result == ERR_OK) {
        platform::setup_complete(node, Health::Nominal);
    } else {
        LOG_ERROR("CAN bus initialization failed: %d", result);
        platform::setup_complete();
    }

    while (true) {
        node.poll();
    }
}
