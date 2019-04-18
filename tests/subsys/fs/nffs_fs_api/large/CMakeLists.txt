# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
if(BOARD STREQUAL nrf52840_pca10056)
    set(CONF_FILE nrf5x.conf)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(large)

if(BOARD STREQUAL qemu_x86)
    zephyr_compile_definitions(
        -DTEST_FLASH_OFFSET=0
    )
elseif(BOARD STREQUAL nrf52840_pca10056)
    zephyr_compile_definitions(
        -DTEST_FLASH_OFFSET=0x20000
    )
endif()

if(NOT TEST)
    target_compile_definitions(app PRIVATE
        -DTEST_basic
	)
else()
    target_compile_definitions(app PRIVATE
        -DTEST_${TEST}
	)
endif()


zephyr_include_directories(../common)
FILE(GLOB app_sources ../common/*.c src/*.c)

if(CONFIG_QEMU_TARGET)
    list(APPEND app_sources ../ram_backend/test_ram_backend.c)
endif()

target_sources(app PRIVATE ${app_sources})
