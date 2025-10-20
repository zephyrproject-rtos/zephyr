# Copyright (c) 2024-2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

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

if(CONFIG_CMAKE_LINKER_GENERATOR)
  zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
  zephyr_linker_section_configure(SECTION .fsp_dtc_vector_table KEEP INPUT ".fsp_dtc_vector_table*")
  zephyr_linker_section(NAME .option_setting_ofs0 GROUP OFS_OFS0_MEMORY ADDRESS ${ofs0_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs0 KEEP INPUT ".option_setting_ofs0*")
  zephyr_linker_section(NAME .option_setting_dualsel GROUP OFS_DUALSEL_MEMORY ADDRESS ${dualsel_addr})
  zephyr_linker_section_configure(SECTION .option_setting_dualsel KEEP INPUT ".option_setting_dualsel*")
  zephyr_linker_section(NAME .option_setting_ofs1_sec GROUP OFS_OFS1_SEC_MEMORY ADDRESS ${ofs1_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1_sec KEEP INPUT ".option_setting_ofs1_sec*")
  zephyr_linker_section(NAME .option_setting_banksel_sec GROUP OFS_BANKSEL_SEC_MEMORY ADDRESS ${banksel_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_banksel_sec KEEP INPUT ".option_setting_banksel_sec*")
  zephyr_linker_section(NAME .option_setting_ofs1_sel GROUP OFS_OFS1_SEL_MEMORY ADDRESS ${ofs1_sel_addr})
  zephyr_linker_section_configure(SECTION .option_setting_ofs1_sel KEEP INPUT ".option_setting_ofs1_sel*")
  zephyr_linker_section(NAME .option_setting_banksel_sel GROUP OFS_BANKSEL_SEL_MEMORY ADDRESS ${banksel_sel_addr})
  zephyr_linker_section_configure(SECTION .option_setting_banksel_sel KEEP INPUT ".option_setting_banksel_sel*")
  zephyr_linker_section(NAME .option_setting_bps_sec GROUP OFS_BPS_SEC_MEMORY ADDRESS ${bps_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_bps_sec KEEP INPUT ".option_setting_bps_sec*")
  zephyr_linker_section(NAME .option_setting_pbps_sec GROUP OFS_PBPS_SEC_MEMORY ADDRESS ${pbps_sec_addr})
  zephyr_linker_section_configure(SECTION .option_setting_pbps_sec KEEP INPUT ".option_setting_pbps_sec*")
  zephyr_linker_section(NAME .option_setting_bps_sel GROUP OFS_BPS_SEL_MEMORY ADDRESS ${bps_sel_addr})
  zephyr_linker_section_configure(SECTION .option_setting_bps_sel KEEP INPUT ".option_setting_bps_sel*")
elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(SECTIONS sections.ld)
  zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)
endif()

if(CONFIG_ETH_RENESAS_RA_USE_NS_BUF)
  # In ra6m4 ethernet peripheral is always non-secure, even in flat project.
  # Use this linker script to place ethernet buffer in non-secure RAM.
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

  if(CONFIG_OUTPUT_RPD)
    # Generate zephyr.rpd file
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${SOC_FULL_DIR}/tools/gen_rpd.py
      --kernel ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}
      --output-rpd ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.rpd
      $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )
  endif()
else()
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
endif()
