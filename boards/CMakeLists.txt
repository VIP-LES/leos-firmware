add_custom_target(firmware)

# LEOS Sensor
file(GLOB SENSOR_SOURCES "${CMAKE_CURRENT_LIST_DIR}/sensor/*.c")
add_executable(leos-sensor ${SENSOR_SOURCES})

target_link_libraries(leos-sensor
    leos_purpleboard
)

target_include_directories(leos-sensor PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/sensor
)

leos_add_board(leos-sensor "LEOS Sensor" "2.1")


# LEOS EFM
file(GLOB EFM_SOURCES "${CMAKE_CURRENT_LIST_DIR}/efm/*.c")
add_executable(leos-efm ${EFM_SOURCES})

target_link_libraries(leos-efm
    leos_mcp342x
)

target_include_directories(leos-efm PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/efm
)

leos_add_board(leos-efm "LEOS EFM" "1.0")

# LEOS Radio
file(GLOB RADIO_SOURCES "${CMAKE_CURRENT_LIST_DIR}/radio/*.c")
add_executable(leos-radio ${RADIO_SOURCES})

target_link_libraries(leos-radio
    leos_sx126x
)

target_include_directories(leos-radio PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/radio
)

leos_add_board(leos-radio "LEOS Radio" "1.0")