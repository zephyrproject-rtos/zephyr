# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(QEMU_EXTRA_FLAGS -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(mesh_demo)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_BOARD_BBC_MICROBIT app PRIVATE src/microbit.c)
zephyr_include_directories_ifdef(CONFIG_BOARD_BBC_MICROBIT
				 $ENV{ZEPHYR_BASE}/boards/arm/bbc_microbit)

if(NODE_ADDR)
  zephyr_compile_definitions(NODE_ADDR=${NODE_ADDR})
endif()
