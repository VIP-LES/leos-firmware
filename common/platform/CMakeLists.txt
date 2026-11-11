add_library(leos_platform STATIC src/platform.cpp)
target_compile_features(leos_platform PUBLIC cxx_std_17)
target_include_directories(leos_platform PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(leos_platform PUBLIC leos_cyphal PRIVATE pico_stdlib leos_log)
