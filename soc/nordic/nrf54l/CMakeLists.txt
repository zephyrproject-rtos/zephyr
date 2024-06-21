# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  soc.c
  ../validate_rram_partitions.c
  )
zephyr_include_directories(.)

# We need a buffer in memory in a static location which can be used by
# the KMU peripheral. The KMU has a static destination address, we chose
# this address to be 0x20000000, which is the first address in the SRAM.
if(NOT CONFIG_BUILD_WITH_TFM AND CONFIG_PSA_NEED_CRACEN_KMU_DRIVER AND CONFIG_XIP)
# Exclamation mark is printable character with the lowest number in ASCII table.
# We are sure that this file will be included first.
zephyr_linker_sources(RAM_SECTIONS SORT_KEY ! kmu_push_area_section.ld)
endif()
