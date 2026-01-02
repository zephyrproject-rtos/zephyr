# Copyright (c) 2024-2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_CMAKE_LINKER_GENERATOR)
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

  dt_node_has_status(ofs0_status PATH ${option_setting_ofs0} STATUS okay)
  dt_node_has_status(osis_status PATH ${option_setting_osis} STATUS okay)
  dt_node_has_status(ofs1_sec_status PATH ${option_setting_ofs1_sec} STATUS okay)
  dt_node_has_status(bps_sec_status PATH ${option_setting_bps_sec} STATUS okay)
  dt_node_has_status(pbps_sec_status PATH ${option_setting_pbps_sec} STATUS okay)

  if(CONFIG_USE_RA_FSP_DTC)
    zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
    zephyr_linker_section_configure(SECTION .fsp_dtc_vector_table KEEP INPUT ".fsp_dtc_vector_table*")
  endif()

  if(${ofs0_status})
    zephyr_linker_section(NAME .option_setting_ofs0 GROUP OFS_OFS0_MEMORY ADDRESS ${ofs0_addr})
    zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")
  endif()

  if(${osis_status})
    zephyr_linker_section(NAME .option_setting_osis GROUP OFS_OSIS_MEMORY ADDRESS ${osis_addr})
    zephyr_linker_section_configure(SECTION .option_setting_osis KEEP INPUT ".option_setting_osis*")
  endif()

  if(${ofs1_sec_status})
    zephyr_linker_section(NAME .option_setting_ofs1_sec GROUP OFS_OFS1_SEC_MEMORY ADDRESS ${ofs1_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_ofs1_sec KEEP INPUT ".option_setting_ofs1_sec*")
  endif()

  if(${bps_sec_status})
    zephyr_linker_section(NAME .option_setting_bps_sec GROUP OFS_BPS_SEC_MEMORY ADDRESS ${bps_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_bps_sec KEEP INPUT ".option_setting_bps_sec*")
  endif()

  if(${pbps_sec_status})
    zephyr_linker_section(NAME .option_setting_pbps_sec GROUP OFS_PBPS_SEC_MEMORY ADDRESS ${pbps_sec_addr})
    zephyr_linker_section_configure(SECTION .option_setting_pbps_sec KEEP INPUT ".option_setting_pbps_sec*")
  endif()

elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(SECTIONS sections.ld)

else()
  message(WARNING "Unsupported linker template.")
endif()
