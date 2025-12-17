# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_MCXW716C mcxw71_platform_init.S)
zephyr_sources_ifdef(CONFIG_SOC_MCXW727C mcxw72_platform_init.S)

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_include_directories(./)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

if(CONFIG_NXP_NBU)
  zephyr_linker_sources(RAM_SECTIONS sections.ld)

  zephyr_linker_section(NAME .noinit_rpmsg_sh_mem VMA rpmsg_sh_mem TYPE NOLOAD NOINIT)
  zephyr_linker_section_configure(
    SECTION .noinit_rpmsg_sh_mem
    INPUT ".noinit.$rpmsg_sh_mem*"
    ALIGN 4
    SYMBOLS __RPMSG_SH_MEM_START__ __RPMSG_SH_MEM_END__
  )
  zephyr_linker_symbol(
    SYMBOL rpmsg_sh_mem_start
    EXPR "@__RPMSG_SH_MEM_START__@"
  )
  zephyr_linker_symbol(
    SYMBOL rpmsg_sh_mem_end
    EXPR "@__RPMSG_SH_MEM_END__@"
  )
endif()
