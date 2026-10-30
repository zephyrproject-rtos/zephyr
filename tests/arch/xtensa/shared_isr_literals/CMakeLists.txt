# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# Place common ROM sections before ordinary text to exercise their literal placement.
set(soc_linker_script "$ENV{ZEPHYR_BASE}/soc/cdns/dc233c/include/xtensa-dc233c.ld")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${soc_linker_script}")
file(READ "${soc_linker_script}" linker_script)
if(NOT linker_script MATCHES "  [.]text : HDR_MMU_PAGE_ALIGN")
  message(FATAL_ERROR "Cannot locate the dc233c text section")
endif()
string(REPLACE "#include <zephyr/linker/common-rom.ld>" "" linker_script "${linker_script}")
string(REPLACE "  .text : HDR_MMU_PAGE_ALIGN"
  "#include <zephyr/linker/common-rom.ld>\n  .text : HDR_MMU_PAGE_ALIGN"
  linker_script "${linker_script}"
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/early-common.ld" "${linker_script}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(shared_isr_literals)

# Function instrumentation makes z_shared_isr emit literal loads even at -Os.
set_source_files_properties("${ZEPHYR_BASE}/arch/common/shared_irq.c"
  TARGET_DIRECTORY arch__common
  PROPERTIES COMPILE_OPTIONS "-finstrument-functions"
)
target_sources(app PRIVATE
  src/main.c
)
