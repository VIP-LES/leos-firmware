#include "leos/firmware.hpp"
#include "publish.hpp"
#include "relays.hpp"

using namespace leos;

constexpr std::uint8_t SENSOR_NODE_ID = 10;

cyphal::Node node{can_hw_config};
SensorPublishing publishing{node};
Relays relays{node};

int main() {
    platform::init();

    relays.initHardware();
    
    const Health health = publishing.init();
    if (node.init(can_config, SENSOR_NODE_ID) != ERR_OK) {
        LOG_ERROR("CAN bus initialization failed");
        platform::fatal();
    }

    Health setup_health = health;
    if (!relays.serve()) {
        LOG_ERROR("Relay service subscription failed");
        setup_health = Health::Caution;
    }
    platform::setup_complete(node, setup_health);

    LOG_INFO("Entering sensor main loop");
    while (true) {
        node.poll();
        publishing.poll();
        relays.poll();
    }
}
