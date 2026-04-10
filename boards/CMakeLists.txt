# LEOS Sensor
file(GLOB SENSOR_SOURCES "${CMAKE_CURRENT_LIST_DIR}/sensor/*.c")
add_executable(leos-sensor-module ${SENSOR_SOURCES})

target_link_libraries(leos-sensor-module
    leos_purpleboard
)

target_include_directories(leos-sensor-module PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/sensor
)

leos_add_board(leos-sensor-module "LEOS Sensor" "2.1")


# LEOS EFM
file(GLOB EFM_SOURCES "${CMAKE_CURRENT_LIST_DIR}/efm/*.c")
add_executable(leos-efm-module ${EFM_SOURCES})

target_link_libraries(leos-efm-module
    leos_mcp342x
)

target_include_directories(leos-efm-module PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/sensor
)

leos_add_board(leos-efm-module "LEOS EFM" "1.0")

