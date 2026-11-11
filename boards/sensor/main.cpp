#include "leos/firmware.hpp"
#include "publish.hpp"

using namespace leos;

constexpr std::uint8_t SENSOR_NODE_ID = 10;

cyphal::Node node{can_hw_config};
SensorPublishing publishing{node};

int main() {
    platform::init();

    const Health health = publishing.init();
    if (node.init(can_config, SENSOR_NODE_ID) != ERR_OK) {
        LOG_ERROR("CAN bus initialization failed");
        platform::fatal();
    }
    platform::setup_complete(node, health);

    LOG_INFO("Entering sensor main loop");
    while (true) {
        node.poll();
        publishing.poll();
    }
}
