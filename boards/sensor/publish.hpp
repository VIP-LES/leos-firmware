#pragma once

#include "leos/firmware.hpp"
#include "leos/purpleboard.hpp"

// Owns the sensor drivers and one Cyphal publisher per sensor subject.
// The Node must outlive this object.
class SensorPublishing final {
public:
    explicit SensorPublishing(leos::cyphal::Node& node);

    // Probe sensors before the node starts; returns the startup heartbeat health.
    leos::Health init();

    // Call after Node::poll() on every main-loop iteration.
    void poll();

private:
    void publishReadings(const leos::Purpleboard::Readings& readings);
    void publishBme(const leos::Purpleboard::Readings& readings);
    void publishTsl(const leos::Purpleboard::Readings& readings);
    void publishLtr(const leos::Purpleboard::Readings& readings);
    void publishPm(const leos::Purpleboard::Readings& readings);

    leos::cyphal::Node& node_;
    leos::Purpleboard purpleboard_{i2c0, 8, 9};
    leos::cyphal::Publisher bme_publisher_;
    leos::cyphal::Publisher tsl_publisher_;
    leos::cyphal::Publisher ltr_publisher_;
    leos::cyphal::Publisher pm_publisher_;
    leos::Health initial_health_ = leos::Health::Nominal;
    bool sampling_ = false;
    absolute_time_t next_sample_{};
};
