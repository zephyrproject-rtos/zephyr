# Copyright (c) 2024-2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
)

zephyr_linker_sources(SECTIONS sections.ld)
zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)

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
