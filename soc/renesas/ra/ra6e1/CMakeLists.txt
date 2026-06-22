# Copyright (c) 2024-2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_CMAKE_LINKER_GENERATOR)
  dt_nodelabel(option_setting_ofs0 NODELABEL "option_setting_ofs0")
  dt_nodelabel(option_setting_dualsel NODELABEL "option_setting_dualsel")
  dt_nodelabel(option_setting_ofs1_sec NODELABEL "option_setting_ofs1_sec")
  dt_nodelabel(option_setting_banksel_sec NODELABEL "option_setting_banksel_sec")
  dt_nodelabel(option_setting_bps_sec NODELABEL "option_setting_bps_sec")
  dt_nodelabel(option_setting_pbps_sec NODELABEL "option_setting_pbps_sec")
  dt_nodelabel(option_setting_ofs1_sel NODELABEL "option_setting_ofs1_sel")
  dt_nodelabel(option_setting_banksel_sel NODELABEL "option_setting_banksel_sel")
  dt_nodelabel(option_setting_bps_sel NODELABEL "option_setting_bps_sel")

  dt_reg_addr(ofs0_addr PATH ${option_setting_ofs0})
  dt_reg_addr(dualsel_addr PATH ${option_setting_dualsel})
  dt_reg_addr(ofs1_sec_addr PATH ${option_setting_ofs1_sec})
  dt_reg_addr(banksel_sec_addr PATH ${option_setting_banksel_sec})
  dt_reg_addr(bps_sec_addr PATH ${option_setting_bps_sec})
  dt_reg_addr(pbps_sec_addr PATH ${option_setting_pbps_sec})
  dt_reg_addr(ofs1_sel_addr PATH ${option_setting_ofs1_sel})
  dt_reg_addr(banksel_sel_addr PATH ${option_setting_banksel_sel})
  dt_reg_addr(bps_sel_addr PATH ${option_setting_bps_sel})

  dt_node_has_status(ofs0_status PATH ${option_setting_ofs0} STATUS okay)
  dt_node_has_status(dualsel_status PATH ${option_setting_dualsel} STATUS okay)
  dt_node_has_status(ofs1_sec_status PATH ${option_setting_ofs1_sec} STATUS okay)
  dt_node_has_status(banksel_sec_status PATH ${option_setting_banksel_sec} STATUS okay)
  dt_node_has_status(bps_sec_status PATH ${option_setting_bps_sec} STATUS okay)
  dt_node_has_status(pbps_sec_status PATH ${option_setting_pbps_sec} STATUS okay)
  dt_node_has_status(ofs1_sel_status PATH ${option_setting_ofs1_sel} STATUS okay)
  dt_node_has_status(banksel_sel_status PATH ${option_setting_banksel_sel} STATUS okay)
  dt_node_has_status(bps_sel_status PATH ${option_setting_bps_sel} STATUS okay)

  if(${ofs0_status})
    zephyr_linker_section(NAME .option_setting_ofs0 GROUP OFS_OFS0_MEMORY ADDRESS ${ofs0_addr})
    zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")
  endif()

  if(${dualsel_status})
    zephyr_linker_section(NAME .option_setting_dualsel GROUP OFS_DUALSEL_MEMORY ADDRESS ${dualsel_addr})
    zephyr_linker_section_configure(SECTION .option_setting_dualsel KEEP INPUT ".option_setting_dualsel*")
  endif()

  if(${ofs1_sec_status})
    zephyr_linker_section(NAME .option_setting_ofs1_sec GROUP OFS_OFS1_SEC_MEMORY ADDRESS ${ofs1_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_ofs1_sec KEEP INPUT ".option_setting_ofs1_sec*")
  endif()

  if(${banksel_sec_status})
    zephyr_linker_section(NAME .option_setting_banksel_sec GROUP OFS_BANKSEL_SEC_MEMORY ADDRESS ${banksel_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_banksel_sec KEEP INPUT ".option_setting_banksel_sec*")
  endif()

  if(${ofs1_sel_status})
    zephyr_linker_section(NAME .option_setting_ofs1_sel GROUP OFS_OFS1_SEL_MEMORY ADDRESS ${ofs1_sel_addr})
    zephyr_linker_section_configure(SECTION .option_setting_ofs1_sel KEEP INPUT ".option_setting_ofs1_sel*")
  endif()

  if(${banksel_sel_status})
    zephyr_linker_section(NAME .option_setting_banksel_sel GROUP OFS_BANKSEL_SEL_MEMORY ADDRESS ${banksel_sel_addr})
    zephyr_linker_section_configure(SECTION .option_setting_banksel_sel KEEP INPUT ".option_setting_banksel_sel*")
  endif()

  if(${bps_sec_status})
    zephyr_linker_section(NAME .option_setting_bps_sec GROUP OFS_BPS_SEC_MEMORY ADDRESS ${bps_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_bps_sec KEEP INPUT ".option_setting_bps_sec*")
  endif()

  if(${pbps_sec_status})
    zephyr_linker_section(NAME .option_setting_pbps_sec GROUP OFS_PBPS_SEC_MEMORY ADDRESS ${pbps_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_pbps_sec KEEP INPUT ".option_setting_pbps_sec*")
  endif()

  if(${bps_sel_status})
    zephyr_linker_section(NAME .option_setting_bps_sel GROUP OFS_BPS_SEL_MEMORY ADDRESS ${bps_sel_addr})
    zephyr_linker_section_configure(SECTION .option_setting_bps_sel KEEP INPUT ".option_setting_bps_sel*")
  endif()

elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(SECTIONS sections.ld)

else()
  message(WARNING "Unsupported linker template.")
endif()
