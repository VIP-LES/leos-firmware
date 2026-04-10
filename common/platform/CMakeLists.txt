add_library(common_platform EXCLUDE_FROM_ALL
    src/platform.c src/config.c
)

# Libraries exposed in public headers
target_link_libraries(common_platform PUBLIC
    leos_mcp251xfd
    leos_cyphal_node
)

# Libraries needed only for setup and inside platform
target_link_libraries(common_platform PRIVATE
    pico_stdlib
    leos_log
    leos_mcp251xfd
    leos_cyphal_transport_mcp251xfd
)

target_include_directories(common_platform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(common_platform PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)