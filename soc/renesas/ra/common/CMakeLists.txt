# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

# Let SystemInit() be called in place of soc_reset_hook() by default.
zephyr_linker_symbol(SYMBOL soc_reset_hook EXPR "@SystemInit@")

# These files are used when the CMake linker script generator is disabled.
if(CONFIG_CMAKE_LINKER_GENERATOR)
  if(CONFIG_USE_RA_FSP_DTC)
    zephyr_linker_section(NAME .fsp_dtc_vector_table GROUP RAM)
    zephyr_linker_section_configure(SECTION .fsp_dtc_vector_table KEEP INPUT ".fsp_dtc_vector_table*")
  endif()

elseif(CONFIG_LD_LINKER_TEMPLATE)
  zephyr_linker_sources(SECTIONS platform_init.ld)
  zephyr_linker_sources(SECTIONS ram_sections.ld)

endif()

zephyr_sources(soc.c)

if(NOT CONFIG_ETH_RENESAS_RA_USE_NS_BUF)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
endif()
