# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mapped_partition)

target_sources(app PRIVATE
  src/main.c
)

if(CONFIG_CMAKE_LINKER_GENERATOR)
  dt_prop(expected_rom_address PATH "/zephyr,user" PROPERTY "expected-rom-address")
  dt_prop(expected_rom_size PATH "/zephyr,user" PROPERTY "expected-rom-size")
  math(EXPR expected_rom_size "${expected_rom_size} - ${CONFIG_ROM_END_OFFSET}")

  zephyr_linker_symbol(SYMBOL test_rom_address
    EXPR "ASSERT(ORIGIN(FLASH) == ${expected_rom_address}, \\\"Unexpected ROM address\\\")"
  )
  zephyr_linker_symbol(SYMBOL test_rom_size
    EXPR "ASSERT(LENGTH(FLASH) == ${expected_rom_size}, \\\"Unexpected ROM size\\\")"
  )
else()
  zephyr_linker_sources(SECTIONS
    rom_assert.ld
  )
endif()
