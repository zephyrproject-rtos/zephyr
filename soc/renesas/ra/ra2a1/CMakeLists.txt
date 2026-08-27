# Copyright (c) 2024 TOKITA Hiroshi
# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_CMAKE_LINKER_GENERATOR)
  dt_nodelabel(option_setting_ofs0 NODELABEL "option_setting_ofs0")
  dt_nodelabel(option_setting_ofs1 NODELABEL "option_setting_ofs1")
  dt_nodelabel(option_setting_secmpu NODELABEL "option_setting_secmpu")
  dt_nodelabel(option_setting_osis NODELABEL "option_setting_osis")

  dt_reg_addr(ofs0_addr PATH ${option_setting_ofs0})
  dt_reg_addr(ofs1_addr PATH ${option_setting_ofs1})
  dt_reg_addr(secmpu_addr PATH ${option_setting_secmpu})
  dt_reg_addr(osis_addr PATH ${option_setting_osis})

  dt_node_has_status(osis_status PATH ${option_setting_osis} STATUS okay)

  zephyr_linker_section(NAME .rom_padding GROUP ROM_REGION ADDRESS 0x000000C0)
  zephyr_linker_section_configure(SECTION .rom_padding KEEP INPUT ".rom_padding*")

  zephyr_linker_section(NAME .option_setting_ofs0 GROUP ROM_REGION ADDRESS ${ofs0_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")

  zephyr_linker_section(NAME .option_setting_ofs1 GROUP ROM_REGION ADDRESS ${ofs1_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1 KEEP INPUT ".option_setting_ofs1*")

  zephyr_linker_section(NAME .option_setting_secmpu GROUP ROM_REGION ADDRESS ${secmpu_addr})
  zephyr_linker_section_configure(SECTION .option_setting_secmpu KEEP INPUT ".option_setting_secmpu*")

  if(${osis_status})
    zephyr_linker_section(NAME .option_setting_osis GROUP OFS_OSIS_MEMORY ADDRESS ${osis_addr})
    zephyr_linker_section_configure(SECTION .option_setting_osis KEEP INPUT ".option_setting_osis*")
  endif()

elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(ROM_START rom_start.ld)
  zephyr_linker_sources(SECTIONS sections.ld)

else()
  message(WARNING "Unsupported linker template.")
endif()
