# Helper function to setup board settings
function(leos_add_board TARGET PRETTY_NAME REVISION)
    leos_add_executable(${TARGET} ${PRETTY_NAME} ${REVISION})

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

    # Output Binary Optimization
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(
            -ffunction-sections
            -fdata-sections
        )

        add_link_options(
            -Wl,--gc-sections
        )
    endif()

    # Include this target in firmware meta group
    add_dependencies(firmware
        ${TARGET}
    )
endfunction()

function(leos_add_tool TARGET PRETTY_NAME REVISION)
    leos_add_executable(${TARGET} ${PRETTY_NAME} ${REVISION})

    # Include common platform in board executables
    target_link_libraries(${TARGET} 
        pico_stdlib 
        leos_log
    )

    # Include this target in firmware meta group
    add_dependencies(tools
        ${TARGET}
    )
endfunction()


# Setup needed for any executable (board firmware & tools)
function(leos_add_executable TARGET PRETTY_NAME REVISION)
    set(FW_DIR "${CMAKE_BINARY_DIR}/firmware")
    set(DEBUG_DIR "${FW_DIR}/debug")

    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${FW_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${FW_DIR}
    )

    pico_set_program_name(${TARGET} ${PRETTY_NAME})
    pico_set_program_version(${TARGET} ${REVISION})

    # Generate .uf2 files for boards
    pico_add_uf2_output(${TARGET})
    
    #Generate .map debug file
    file(MAKE_DIRECTORY ${DEBUG_DIR})
    target_link_options(${TARGET} PRIVATE
        "LINKER:-Map=${DEBUG_DIR}/$<TARGET_FILE_BASE_NAME:${TARGET}>.map"
    )

    # Modify the below lines to enable/disable output over UART/USB
    pico_enable_stdio_uart(${TARGET} 0)
    pico_enable_stdio_usb(${TARGET} 1)
endfunction()