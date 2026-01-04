# Set minimum CMake version
cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_adxl345_accel_convert)

target_include_directories(testbinary PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../
)

target_sources(testbinary
    PRIVATE
    test_adxl345_accel_convert.c
    ${CMAKE_CURRENT_LIST_DIR}/../../adxl345.c
)
