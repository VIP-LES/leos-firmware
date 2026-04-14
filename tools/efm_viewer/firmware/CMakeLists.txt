#LEOS EFM Test
file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.c")
add_executable(leos-efm-viewer ${SOURCES})

target_link_libraries(leos-efm-viewer
    leos_mcp342x
)

target_include_directories(leos-efm-viewer PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

leos_add_tool(leos-efm-viewer "LEOS EFM ADC Viewer" "1.0")