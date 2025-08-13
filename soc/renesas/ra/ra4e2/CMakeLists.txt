# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

dt_nodelabel(option_setting_ofs0 NODELABEL "option_setting_ofs0")
dt_nodelabel(option_setting_osis NODELABEL "option_setting_osis")
dt_nodelabel(option_setting_ofs1_sec NODELABEL "option_setting_ofs1_sec")
dt_nodelabel(option_setting_bps_sec NODELABEL "option_setting_bps_sec")
dt_nodelabel(option_setting_pbps_sec NODELABEL "option_setting_pbps_sec")
dt_reg_addr(ofs0_addr PATH ${option_setting_ofs0})
dt_reg_addr(osis_addr PATH ${option_setting_osis})
dt_reg_addr(ofs1_sec_addr PATH ${option_setting_ofs1_sec})
dt_reg_addr(bps_sec_addr PATH ${option_setting_bps_sec})
dt_reg_addr(pbps_sec_addr PATH ${option_setting_pbps_sec})

if(CONFIG_CMAKE_LINKER_GENERATOR)
  zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
  zephyr_linker_section(NAME .option_setting_ofs0 GROUP OFS_CONF_MEMORY ADDRESS ${ofs0_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0")
  zephyr_linker_section(NAME .option_setting_osis GROUP OFS_CONF_MEMORY ADDRESS ${osis_addr})
  zephyr_linker_section_configure(SECTION .option_setting_osis KEEP INPUT ".option_setting_osis")
  zephyr_linker_section(NAME .option_setting_ofs1_sec GROUP OFS_CONF_MEMORY ADDRESS ${ofs1_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1_sec KEEP INPUT ".option_setting_ofs1_sec")
  zephyr_linker_section(NAME .option_setting_bps_sec GROUP OFS_CONF_MEMORY ADDRESS ${bps_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_bps_sec KEEP INPUT ".option_setting_bps_sec")
  zephyr_linker_section(NAME .option_setting_pbps_sec GROUP OFS_CONF_MEMORY ADDRESS ${pbps_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_pbps_sec KEEP INPUT ".option_setting_pbps_sec")
elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(SECTIONS sections.ld)
  zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
