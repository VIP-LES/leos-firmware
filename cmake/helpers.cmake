# Helper function to setup board settings
function(leos_add_board TARGET PRETTY_NAME REVISION)

    pico_set_program_name(${TARGET} ${PRETTY_NAME})
    pico_set_program_version(${TARGET} ${REVISION})

    # Generate our custom Cyphal types
    cyphal_generate_types(
        TARGET ${TARGET}
        DSDL_DIR ${PROJECT_SOURCE_DIR}/external/leos-cyphal-types/leos
        DSDL_LOOKUP_DIRS ${PROJECT_SOURCE_DIR}/external/leos-sdk/external/public_regulated_data_types/uavcan
    )

    # Include common platform in board executables
    target_link_libraries(${TARGET} 
        common_platform 
        pico_stdlib 
        leos_log
    )

    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_uart(${TARGET} 0)
    pico_enable_stdio_usb(${TARGET} 1)

    # Generate .uf2 files for boards
    pico_add_extra_outputs(${TARGET})
endfunction()