# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

if(CONFIG_CMAKE_LINKER_GENERATOR)
  zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
  zephyr_linker_section(NAME .option_setting_ofs0 GROUP OPTION_SETTING_OFS ADDRESS 0x0100a100)
  zephyr_linker_section(NAME .option_setting_sas GROUP OPTION_SETTING_SAS ADDRESS 0x0100a134)
  zephyr_linker_section(NAME .option_setting_ofs1_sec GROUP OPTION_SETTING_S ADDRESS 0x0100a200)
  zephyr_linker_section(NAME .option_setting_bps_sec0 GROUP OPTION_SETTING_S ADDRESS 0x0100a240)
  zephyr_linker_section(NAME .option_setting_pbps_sec0 GROUP OPTION_SETTING_S ADDRESS 0x0100a260)
  zephyr_linker_section(NAME .id_code GROUP ID_CODE)
else()
  zephyr_linker_sources_ifdef(CONFIG_LD_LINKER_TEMPLATE SECTIONS sections.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
