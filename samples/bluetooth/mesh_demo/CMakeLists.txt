# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(QEMU_EXTRA_FLAGS -s)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_demo)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_BOARD_BBC_MICROBIT app PRIVATE src/microbit.c)
zephyr_include_directories_ifdef(CONFIG_BOARD_BBC_MICROBIT
				 ${ZEPHYR_BASE}/boards/arm/bbc_microbit)

if(NODE_ADDR)
  zephyr_compile_definitions(NODE_ADDR=${NODE_ADDR})
endif()
