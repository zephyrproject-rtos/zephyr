# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mapped_partition)

target_sources(app PRIVATE
  src/main.c
)

if(TEST_OVERFLOW_IMAGE)
  target_sources(app PRIVATE
    src/overflow.c
  )
endif()

if(CONFIG_CMAKE_LINKER_GENERATOR)
  dt_prop(expected_rom_address PATH "/zephyr,user" PROPERTY "expected-rom-address")

  zephyr_linker_symbol(SYMBOL test_rom_address
    EXPR "ASSERT(ORIGIN(FLASH) == ${expected_rom_address}, \\\"Unexpected ROM address\\\")"
  )
  if(NOT TEST_OVERFLOW_IMAGE)
    dt_prop(expected_rom_size PATH "/zephyr,user" PROPERTY "expected-rom-size")
    math(EXPR expected_rom_size "${expected_rom_size} - ${CONFIG_ROM_END_OFFSET}")

    zephyr_linker_symbol(SYMBOL test_rom_size
      EXPR "ASSERT(LENGTH(FLASH) == ${expected_rom_size}, \\\"Unexpected ROM size\\\")"
    )
  endif()
else()
  zephyr_linker_sources(SECTIONS
    rom_assert.ld
  )
  if(NOT TEST_OVERFLOW_IMAGE)
    zephyr_linker_sources(SECTIONS
      rom_size_assert.ld
    )
  endif()
endif()

if(TEST_OVERFLOW_CHECK)
  enable_testing()
  include(CTest)
  configure_file(check_overflow.cmake.in check_overflow.cmake @ONLY)
  add_test(NAME mapped_partition_storage_overflow
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/check_overflow.cmake"
  )
endif()
