# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

zephyr_linker_sources(
  ROM_START
  ${CMAKE_CURRENT_SOURCE_DIR}/ofs.ld
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
