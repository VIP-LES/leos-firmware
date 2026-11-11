#include "publish.hpp"

#include <leos/sensors/BME688_0_1.h>
#include <leos/sensors/LTR390_0_1.h>
#include <leos/sensors/PMSA003I_0_1.h>
#include <leos/sensors/TSL2591_0_1.h>

#include <cstring>

using namespace leos;

static constexpr std::uint32_t SAMPLE_PERIOD_MS = 1000;

static void set_location(uavcan_primitive_String_1_0& location) {
    constexpr char name[] = "purpleboard";
    static_assert(sizeof(name) - 1 <= sizeof(location.value.elements));
    location.value.count = sizeof(name) - 1;
    std::memcpy(location.value.elements, name, location.value.count);
}

static void queue_message(cyphal::Publisher& publisher, const char* name,
                          const std::uint8_t* buffer, std::size_t size) {
    if (publisher.publish(buffer, size) != cyphal::Publisher::Result::Queued) {
        LOG_WARNING("Could not queue %s reading", name);
    }
}
SensorPublishing::SensorPublishing(cyphal::Node& node)
    : node_(node),
      bme_publisher_(node, leos_sensors_BME688_0_1_FIXED_PORT_ID_),
      tsl_publisher_(node, leos_sensors_TSL2591_0_1_FIXED_PORT_ID_),
      ltr_publisher_(node, leos_sensors_LTR390_0_1_FIXED_PORT_ID_),
      pm_publisher_(node, leos_sensors_PMSA003I_0_1_FIXED_PORT_ID_) {}

Health SensorPublishing::init() {
    if (purpleboard_.init() != Purpleboard::Result::Ok) {
        LOG_WARNING("Some Purpleboard sensors were not detected");
        initial_health_ = Health::Caution;
    }
    next_sample_ = get_absolute_time();
    return initial_health_;
}

void SensorPublishing::poll() {
    if (!sampling_ && time_reached(next_sample_)) {
        if (purpleboard_.startSampling()) {
            sampling_ = true;
            next_sample_ = make_timeout_time_ms(SAMPLE_PERIOD_MS);
        }
    }

    if (!sampling_ || !purpleboard_.samplingReady()) {
        return;
    }

    Purpleboard::Readings readings{};
    const auto result = purpleboard_.finishSampling(readings);
    sampling_ = false;
    if (result == Purpleboard::Result::SensorReadDegraded) {
        LOG_DEBUG("Purpleboard sample has invalid sensor readings");
    }
    if (result == Purpleboard::Result::Ok ||
        result == Purpleboard::Result::SensorReadDegraded) {
        node_.setHealth(result == Purpleboard::Result::Ok
                            ? initial_health_ : Health::Caution);
        publishReadings(readings);
    }
}

void SensorPublishing::publishBme(const Purpleboard::Readings& readings) {
    leos_sensors_BME688_0_1 message{};
    set_location(message.location);
    message.valid = readings.bme680_valid;
    message.humidity = readings.humidity;
    message.pressure.pascal = readings.pressure_mb * 100.0f;
    message.temperature.kelvin = readings.temperature_c + 273.15f;
    message.altitude.meter = readings.altitude_m;
    message.gas_resistance = static_cast<float>(readings.gas_resistance);

    std::uint8_t buffer[leos_sensors_BME688_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    std::size_t size = sizeof(buffer);
    if (leos_sensors_BME688_0_1_serialize_(&message, buffer, &size) == 0) {
        queue_message(bme_publisher_, "BME688", buffer, size);
    } else {
        LOG_WARNING("Could not serialize BME688 reading");
    }
}

void SensorPublishing::publishTsl(const Purpleboard::Readings& readings) {
    leos_sensors_TSL2591_0_1 message{};
    set_location(message.location);
    message.valid = readings.tsl2591_valid;
    message.light_lux = readings.lux;
    message.raw_visible = readings.raw_visible;
    message.raw_infrared = readings.raw_infrared;
    message.raw_full_spectrum = readings.raw_full_spectrum;

    std::uint8_t buffer[leos_sensors_TSL2591_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    std::size_t size = sizeof(buffer);
    if (leos_sensors_TSL2591_0_1_serialize_(&message, buffer, &size) == 0) {
        queue_message(tsl_publisher_, "TSL2591", buffer, size);
    } else {
        LOG_WARNING("Could not serialize TSL2591 reading");
    }
}

void SensorPublishing::publishLtr(const Purpleboard::Readings& readings) {
    leos_sensors_LTR390_0_1 message{};
    set_location(message.location);
    message.valid = readings.ltr390_valid;
    message.uvs = readings.ltr390_uvs;

    std::uint8_t buffer[leos_sensors_LTR390_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    std::size_t size = sizeof(buffer);
    if (leos_sensors_LTR390_0_1_serialize_(&message, buffer, &size) == 0) {
        queue_message(ltr_publisher_, "LTR390", buffer, size);
    } else {
        LOG_WARNING("Could not serialize LTR390 reading");
    }
}

void SensorPublishing::publishPm(const Purpleboard::Readings& readings) {
    leos_sensors_PMSA003I_0_1 message{};
    set_location(message.location);
    message.valid = readings.pmsa003i_valid;
    message.pm10_env = readings.pm10_env;
    message.pm25_env = readings.pm25_env;
    message.pm100_env = readings.pm100_env;
    message.aqi_pm25_us = readings.aqi_pm25_us;
    message.aqi_pm100_us = readings.aqi_pm100_us;
    message.particles_03um = readings.particles_03um;
    message.particles_05um = readings.particles_05um;
    message.particles_10um = readings.particles_10um;
    message.particles_25um = readings.particles_25um;
    message.particles_50um = readings.particles_50um;
    message.particles_100um = readings.particles_100um;

    std::uint8_t buffer[leos_sensors_PMSA003I_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    std::size_t size = sizeof(buffer);
    if (leos_sensors_PMSA003I_0_1_serialize_(&message, buffer, &size) == 0) {
        queue_message(pm_publisher_, "PMSA003I", buffer, size);
    } else {
        LOG_WARNING("Could not serialize PMSA003I reading");
    }
}

void SensorPublishing::publishReadings(const Purpleboard::Readings& readings) {
    publishBme(readings);
    publishTsl(readings);
    publishLtr(readings);
    publishPm(readings);
}
