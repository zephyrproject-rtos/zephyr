# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_CMAKE_LINKER_GENERATOR)
  dt_nodelabel(option_setting_ofs0 NODELABEL "option_setting_ofs0")
  dt_nodelabel(option_setting_ofs1 NODELABEL "option_setting_ofs1")
  dt_nodelabel(option_setting_osis NODELABEL "option_setting_osis")

  dt_reg_addr(ofs0_addr PATH ${option_setting_ofs0})
  dt_reg_addr(ofs1_addr PATH ${option_setting_ofs1})
  dt_reg_addr(osis_addr PATH ${option_setting_osis})

  dt_node_has_status(osis_status PATH ${option_setting_osis} STATUS okay)

  zephyr_linker_section(NAME .option_setting_ofs0 GROUP ROM_REGION ADDRESS ${ofs0_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")

  zephyr_linker_section(NAME .option_setting_ofs1 GROUP ROM_REGION ADDRESS ${ofs1_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1 KEEP INPUT ".option_setting_ofs1*")

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
