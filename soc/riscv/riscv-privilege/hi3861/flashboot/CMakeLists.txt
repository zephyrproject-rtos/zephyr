# Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(flashboot C ASM)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

add_executable(flashboot)

generate_binary(flashboot)

target_sources(flashboot PRIVATE riscv_init.S)
target_sources(flashboot PRIVATE main.c)

target_compile_options(flashboot PRIVATE -DCONFIG_FLASH_BASE_ADDRESS=${CONFIG_FLASH_BASE_ADDRESS})
target_compile_options(flashboot PRIVATE -DCONFIG_FLASH_LOAD_OFFSET=${CONFIG_FLASH_LOAD_OFFSET})

add_custom_command(
  TARGET  flashboot PRE_LINK
  COMMAND ${CMAKE_C_COMPILER}
  ARGS    -x assembler-with-cpp
          -DCONFIG_FLASH_BASE_ADDRESS=${CONFIG_FLASH_BASE_ADDRESS}
          -DCONFIG_FLASH_LOAD_OFFSET=${CONFIG_FLASH_LOAD_OFFSET}
          -E ${CMAKE_CURRENT_SOURCE_DIR}/flashboot.lds
          -P
          -o ${CMAKE_CURRENT_BINARY_DIR}/flashboot.ld
)

target_link_options(flashboot PRIVATE -T ${CMAKE_CURRENT_BINARY_DIR}/flashboot.ld)
target_link_options(flashboot PRIVATE -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/flashboot.map)
