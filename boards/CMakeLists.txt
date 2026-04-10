# LEOS Sensor
file(GLOB SENSOR_SOURCES "${CMAKE_CURRENT_LIST_DIR}/sensor/*.c")
add_executable(leos-sensor-module ${SENSOR_SOURCES})

target_link_libraries(leos-sensor-module
    pico_stdlib
    leos_log
    leos_purpleboard
)

target_include_directories(leos-sensor-module PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/sensor
)

leos_add_board(leos-sensor-module "LEOS Sensor" "2.1")
