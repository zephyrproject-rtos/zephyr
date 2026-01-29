# Set minimum CMake version
cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_adxl345_accel_convert)

target_sources(testbinary PRIVATE src/main.c)
