#include "common/platform.h"
#include "purpleboard.h"

#include <leos/sensors/BME688_0_1.h>
#include <leos/sensors/LTR390_0_1.h>
#include <leos/sensors/PMSA003I_0_1.h>
#include <leos/sensors/TSL2591_0_1.h>

#include <string.h>

leos_purpleboard_t *pb = NULL;
bool pb_initialized = false;

const char* location_name = "purpleboard";

static inline float c_to_k(float c) {
    return c + 273.15f;
}

static void set_cyphal_string(uavcan_primitive_String_1_0* string, const char* value) {
    string->value.count = strlen(location_name);
    memcpy(string->value.elements, location_name, string->value.count);
}

static void publish_bme688(const leos_purpleboard_readings_t *readings, leos_cyphal_node_t *node) {
    LOG_INFO(
        "BME688: valid=%d temp=%.2fC pressure=%.2fmb humidity=%.2f gas=%u @ %s",
        readings->bme680_valid,
        readings->temperature_c,
        readings->pressure_mb,
        readings->humidity,
        readings->gas_resistance,
        location_name
    );

    static CanardTransferID transfer_id = 0;
    leos_sensors_BME688_0_1 message = {
        .valid = readings->bme680_valid,
        .humidity = readings->humidity,
        .pressure.pascal = readings->pressure_mb * 100.0f,
        .temperature.kelvin = c_to_k(readings->temperature_c),
        .altitude.meter = readings->altitude_m,
        .gas_resistance = (float) readings->gas_resistance,
    };
    set_cyphal_string(&message.location, location_name);

    uint8_t buffer[leos_sensors_BME688_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_sensors_BME688_0_1_serialize_(&message, buffer, &size) != 0) {
        LOG_DEBUG("Failed to serialize the BME688 message. Skipping.");
        return;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_sensors_BME688_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .remote_node_id = CANARD_NODE_ID_UNSET,
        .transfer_id = transfer_id++,
        .transfer_kind = CanardTransferKindMessage
    };
    const struct CanardPayload payload = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(node, &md, payload) != LEOS_CYPHAL_OK) {
        LOG_DEBUG("Couldn't push BME688 message");
    }
}

static void publish_tsl2591(const leos_purpleboard_readings_t *readings, leos_cyphal_node_t *node) {
    LOG_INFO(
        "TSL2591: valid=%d lux=%.2f visible=%u infrared=%u full=%u @ %s",
        readings->tsl2591_valid,
        readings->lux,
        readings->raw_visible,
        readings->raw_infrared,
        readings->raw_full_spectrum,
        location_name
    );

    static CanardTransferID transfer_id = 0;
    leos_sensors_TSL2591_0_1 message = {
        .valid = readings->tsl2591_valid,
        .light_lux = readings->lux,
        .raw_visible = readings->raw_visible,
        .raw_infrared = readings->raw_infrared,
        .raw_full_spectrum = readings->raw_full_spectrum,
    };
    set_cyphal_string(&message.location, location_name);

    uint8_t buffer[leos_sensors_TSL2591_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_sensors_TSL2591_0_1_serialize_(&message, buffer, &size) != 0) {
        LOG_DEBUG("Failed to serialize the TSL2591 message. Skipping.");
        return;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_sensors_TSL2591_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .remote_node_id = CANARD_NODE_ID_UNSET,
        .transfer_id = transfer_id++,
        .transfer_kind = CanardTransferKindMessage
    };
    const struct CanardPayload payload = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(node, &md, payload) != LEOS_CYPHAL_OK) {
        LOG_DEBUG("Couldn't push TSL2591 message");
    }
}

static void publish_ltr390(const leos_purpleboard_readings_t *readings, leos_cyphal_node_t *node) {
    LOG_INFO("LTR390: valid=%d uvs=%u @ %s", readings->ltr390_valid, readings->ltr390_uvs, location_name);

    static CanardTransferID transfer_id = 0;
    leos_sensors_LTR390_0_1 message = {
        .valid = readings->ltr390_valid,
        .uvs = readings->ltr390_uvs,
    };
    set_cyphal_string(&message.location, location_name);

    uint8_t buffer[leos_sensors_LTR390_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_sensors_LTR390_0_1_serialize_(&message, buffer, &size) != 0) {
        LOG_DEBUG("Failed to serialize the LTR390 message. Skipping.");
        return;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_sensors_LTR390_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .remote_node_id = CANARD_NODE_ID_UNSET,
        .transfer_id = transfer_id++,
        .transfer_kind = CanardTransferKindMessage
    };
    const struct CanardPayload payload = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(node, &md, payload) != LEOS_CYPHAL_OK) {
        LOG_DEBUG("Couldn't push LTR390 message");
    }
}

static void publish_pmsa003i(const leos_purpleboard_readings_t *readings, leos_cyphal_node_t *node) {
    LOG_INFO(
        "PMSA003I: valid=%d pm1=%u pm2.5=%u pm10=%u particles_03=%u @ %s",
        readings->pmsa003i_valid,
        readings->pm10_env,
        readings->pm25_env,
        readings->pm100_env,
        readings->particles_03um,
        location_name
    );

    static CanardTransferID transfer_id = 0;
    leos_sensors_PMSA003I_0_1 message = {
        .valid = readings->pmsa003i_valid,
        .pm10_env = readings->pm10_env,
        .pm25_env = readings->pm25_env,
        .pm100_env = readings->pm100_env,
        .aqi_pm25_us = readings->aqi_pm25_us,
        .aqi_pm100_us = readings->aqi_pm100_us,
        .particles_03um = readings->particles_03um,
        .particles_05um = readings->particles_05um,
        .particles_10um = readings->particles_10um,
        .particles_25um = readings->particles_25um,
        .particles_50um = readings->particles_50um,
        .particles_100um = readings->particles_100um,
    };
    set_cyphal_string(&message.location, location_name);

    uint8_t buffer[leos_sensors_PMSA003I_0_1_SERIALIZATION_BUFFER_SIZE_BYTES_];
    size_t size = sizeof(buffer);
    if (leos_sensors_PMSA003I_0_1_serialize_(&message, buffer, &size) != 0) {
        LOG_DEBUG("Failed to serialize the PMSA003I message. Skipping.");
        return;
    }

    const struct CanardTransferMetadata md = {
        .port_id = leos_sensors_PMSA003I_0_1_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .remote_node_id = CANARD_NODE_ID_UNSET,
        .transfer_id = transfer_id++,
        .transfer_kind = CanardTransferKindMessage
    };
    const struct CanardPayload payload = {
        .data = buffer,
        .size = size
    };
    if (leos_cyphal_push(node, &md, payload) != LEOS_CYPHAL_OK) {
        LOG_DEBUG("Couldn't push PMSA003I message");
    }
}

void purpleboard_task() {
    if (&leos_node == NULL) {
        return;
    }

    // Do nothing if purpleboard is not correctly setup
    if (!pb_initialized) return;

    static absolute_time_t next_run = 0;
    absolute_time_t now = get_absolute_time();
    if (now < next_run) {
        return;
    }
    next_run = make_timeout_time_ms(PB_DATA_INTERVAL_MS);

    LOG_TRACE("Collecting purpleboard measurements");
    leos_purpleboard_readings_t data;
    const leos_purpleboard_result_t err = leos_purpleboard_read(pb, &data);
    if ((err != PB_OK) && (err != PB_SENSOR_READ_DEGRADED)) {
        LOG_ERROR("Failed to read from purpleboard sensors");
        return;
    }

    publish_bme688(&data, &leos_node);
    publish_tsl2591(&data, &leos_node);
    publish_ltr390(&data, &leos_node);
    publish_pmsa003i(&data, &leos_node);
}
