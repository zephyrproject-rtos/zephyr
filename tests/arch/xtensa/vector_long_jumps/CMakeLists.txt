# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

set(soc_linker_script "$ENV{ZEPHYR_BASE}/soc/cdns/dc233c/include/xtensa-dc233c.ld")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${soc_linker_script}")
file(READ "${soc_linker_script}" linker_script)
if(NOT linker_script MATCHES "    [*][(][.]iram0.literal")
  message(FATAL_ERROR "Cannot locate the dc233c vector helper sections")
endif()
# Keep vector helpers beyond the signed 18-bit direct jump range.
string(REPLACE "    *(.iram0.literal"
  "    . += 0x40000;\n    *(.iram0.literal"
  linker_script "${linker_script}"
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/distant-vectors.ld" "${linker_script}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vector_long_jumps)

target_sources(app PRIVATE
  src/main.c
)

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/arch/xtensa/include")
