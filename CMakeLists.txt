cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2)

# add CMake directory in path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import)

project(leos-firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include(leos-sdk)
include(helpers)

add_subdirectory(boards)
add_subdirectory(common)
add_subdirectory(tools)