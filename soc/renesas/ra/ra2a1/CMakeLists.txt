# Copyright (c) 2024 TOKITA Hiroshi
# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

dt_nodelabel(option_setting_ofs0 NODELABEL "option_setting_ofs0")
dt_nodelabel(option_setting_ofs1 NODELABEL "option_setting_ofs1")
dt_nodelabel(option_setting_secmpu NODELABEL "option_setting_secmpu")
dt_nodelabel(option_setting_osis NODELABEL "option_setting_osis")

dt_reg_addr(ofs0_addr PATH ${option_setting_ofs0})
dt_reg_addr(ofs1_addr PATH ${option_setting_ofs1})
dt_reg_addr(secmpu_addr PATH ${option_setting_secmpu})
dt_reg_addr(osis_addr PATH ${option_setting_osis})

if(CONFIG_CMAKE_LINKER_GENERATOR)
  zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
  zephyr_linker_section_configure(SECTION .fsp_dtc_vector_table KEEP INPUT ".fsp_dtc_vector_table*")
  zephyr_linker_section(NAME .option_setting_ofs0 GROUP FLASH ADDRESS ${ofs0_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")
  zephyr_linker_section(NAME .option_setting_ofs1 GROUP FLASH ADDRESS ${ofs1_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1 KEEP INPUT ".option_setting_ofs1*")
  zephyr_linker_section(NAME .option_setting_secmpu GROUP FLASH ADDRESS ${secmpu_addr})
  zephyr_linker_section_configure(SECTION .option_setting_secmpu KEEP INPUT ".option_setting_secmpu*")
  zephyr_linker_section(NAME .option_setting_osis GROUP OFS_OSIS_MEMORY ADDRESS ${osis_addr})
  zephyr_linker_section_configure(SECTION .option_setting_osis KEEP INPUT ".option_setting_osis*")
elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(ROM_START rom_start.ld)
  zephyr_linker_sources(SECTIONS sections.ld)
  zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
