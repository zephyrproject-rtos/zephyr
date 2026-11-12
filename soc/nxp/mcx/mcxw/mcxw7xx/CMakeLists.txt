# Copyright 2025-2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(soc_dcdc.c)
zephyr_sources_ifdef(CONFIG_SOC_MCXW727C mcxw72_trdc.c)
zephyr_sources_ifdef(CONFIG_SOC_MCXW70AC mcxw70_platform_init.S)
zephyr_sources_ifdef(CONFIG_SOC_MCXW716C mcxw71_platform_init.S)
zephyr_sources_ifdef(CONFIG_SOC_MCXW727C mcxw72_platform_init.S)

zephyr_sources_ifdef(CONFIG_PM power.c)

if(CONFIG_PM_S2RAM)
  if(CONFIG_SOC_MCXW716C OR CONFIG_SOC_MCXW727C)
    set(s2ram_layout_checker "${CMAKE_CURRENT_SOURCE_DIR}/check_s2ram_layout.py")
    dt_nodelabel(s2ram_reserved_node NODELABEL "s2ram_reserved" REQUIRED)
    set(s2ram_regions)
    foreach(alias secure nonsecure)
      dt_reg_addr(s2ram_base PATH "${s2ram_reserved_node}" NAME "${alias}")
      dt_reg_size(s2ram_size PATH "${s2ram_reserved_node}" NAME "${alias}")
      if(NOT DEFINED s2ram_base OR NOT DEFINED s2ram_size)
        message(FATAL_ERROR "S2RAM requires the ${alias} wake-workspace range in DT")
      endif()
      list(APPEND s2ram_regions --region "${s2ram_base}" "${s2ram_size}")
    endforeach()
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND "${PYTHON_EXECUTABLE}" "${s2ram_layout_checker}" "$<TARGET_FILE:zephyr_final>"
        ${s2ram_regions}
    )
    set_property(TARGET zephyr_interface APPEND PROPERTY
      INTERFACE_LINK_DEPENDS "${s2ram_layout_checker}"
    )
  endif()
endif()

zephyr_include_directories(./)

zephyr_linker_sources_ifdef(CONFIG_NXP_MCXW7XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

zephyr_linker_sources(RAM_SECTIONS sections.ld)
