# Set minimum CMake version
cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_adxl345_accel_convert)

target_sources(testbinary
    PRIVATE
        src/main.c
        $ENV{ZEPHYR_BASE}/drivers/sensor/adi/adxl345/adxl345_convert.c
)

# make sure we include prepared sensor.h and have access to adxl345_features.h
target_include_directories(testbinary
    BEFORE
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $ENV{ZEPHYR_BASE}/drivers/sensor/adi/adxl345
)
