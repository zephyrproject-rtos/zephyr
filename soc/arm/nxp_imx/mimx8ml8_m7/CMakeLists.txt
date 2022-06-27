#
# Copyright (c) 2021, Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_sources(
  soc.c
  mpu_regions.c
)

if(CONFIG_OPENAMP_RSC_TABLE)
  zephyr_linker_section(NAME .resource_table GROUP ROM_REGION NOINPUT)
  zephyr_linker_section_configure(SECTION .resource_table KEEP INPUT ".resource_table*")
endif()
