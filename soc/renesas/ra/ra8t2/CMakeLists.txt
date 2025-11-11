# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

zephyr_sources_ifdef(CONFIG_PM
  power.c
)

dt_nodelabel(option_setting_ofs0 NODELABEL "option_setting_ofs0")
dt_nodelabel(option_setting_ofs2 NODELABEL "option_setting_ofs2")
dt_nodelabel(option_setting_sas NODELABEL "option_setting_sas")
dt_nodelabel(option_setting_ofs1_sec NODELABEL "option_setting_ofs1_sec")
dt_nodelabel(option_setting_ofs3_sec NODELABEL "option_setting_ofs3_sec")
dt_nodelabel(option_setting_ofs1_sel NODELABEL "option_setting_ofs1_sel")
dt_nodelabel(option_setting_ofs3_sel NODELABEL "option_setting_ofs3_sel")
dt_nodelabel(option_setting_bps_sec NODELABEL "option_setting_bps_sec")
dt_nodelabel(option_setting_otp_pbps_sec NODELABEL "option_setting_otp_pbps_sec")

dt_reg_addr(ofs0_addr PATH ${option_setting_ofs0})
dt_reg_addr(ofs2_addr PATH ${option_setting_ofs2})
dt_reg_addr(sas_addr PATH ${option_setting_sas})
dt_reg_addr(ofs1_sec_addr PATH ${option_setting_ofs1_sec})
dt_reg_addr(ofs3_sec_addr PATH ${option_setting_ofs3_sec})
dt_reg_addr(ofs1_sel_addr PATH ${option_setting_ofs1_sel})
dt_reg_addr(ofs3_sel_addr PATH ${option_setting_ofs3_sel})
dt_reg_addr(bps_sec_addr PATH ${option_setting_bps_sec})
dt_reg_addr(otp_pbps_sec_addr PATH ${option_setting_otp_pbps_sec})

if(CONFIG_CMAKE_LINKER_GENERATOR)
  zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
  zephyr_linker_section_configure(SECTION .fsp_dtc_vector_table KEEP INPUT ".fsp_dtc_vector_table*")
  zephyr_linker_section(NAME .option_setting_ofs0 GROUP OFS_OFS0_MEMORY ADDRESS ${ofs0_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")
  zephyr_linker_section(NAME .option_setting_ofs2 GROUP OFS_OFS2_MEMORY ADDRESS ${ofs2_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs2 KEEP INPUT ".option_setting_ofs2*")
  zephyr_linker_section(NAME .option_setting_sas GROUP OFS_SAS_MEMORY ADDRESS ${sas_addr})
  zephyr_linker_section_configure(SECTION .option_setting_sas KEEP INPUT ".option_setting_sas*")
  zephyr_linker_section(NAME .option_setting_ofs1_sec GROUP OFS_OFS1_SEC_MEMORY ADDRESS ${ofs1_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1_sec KEEP INPUT ".option_setting_ofs1_sec*")
  zephyr_linker_section(NAME .option_setting_ofs3_sec GROUP OFS_OFS3_SEC_MEMORY ADDRESS ${ofs3_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs3_sec KEEP INPUT ".option_setting_ofs3_sec*")
  zephyr_linker_section(NAME .option_setting_ofs1_sel GROUP OFS_OFS1_SEL_MEMORY ADDRESS ${ofs1_sel_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1_sel KEEP INPUT ".option_setting_ofs1_sel*")
  zephyr_linker_section(NAME .option_setting_ofs3_sel GROUP OFS_OFS3_SEL_MEMORY ADDRESS ${ofs3_sel_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs3_sel KEEP INPUT ".option_setting_ofs3_sel*")
  zephyr_linker_section(NAME .option_setting_bps_sec GROUP OFS_BPS_SEC_MEMORY ADDRESS ${bps_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_bps_sec KEEP INPUT ".option_setting_bps_sec*")
  zephyr_linker_section(NAME .option_setting_otp_pbps_sec GROUP OFS_OTP_PBPS_SEC_MEMORY ADDRESS ${otp_pbps_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_otp_pbps_sec KEEP INPUT ".option_setting_otp_pbps_sec*")
elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(SECTIONS sections.ld)
  zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
