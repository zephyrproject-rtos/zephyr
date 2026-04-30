# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esp32s3_box3_demo)

target_sources(app PRIVATE src/main.c)

# Add test audio data if the file exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_audio.c)
    target_sources(app PRIVATE test_audio.c)
    target_compile_definitions(app PRIVATE TEST_AUDIO_AVAILABLE=1)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

