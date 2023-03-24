# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(BOARD STREQUAL qemu_x86_64)
	LIST(APPEND QEMU_EXTRA_FLAGS -machine q35)
endif()

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/smbus)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smbus_api)

target_sources(app PRIVATE src/test_smbus.c)
target_sources_ifdef(CONFIG_BOARD_QEMU_X86_64 app PRIVATE src/test_smbus_qemu.c)
