# Copyright (c) 2026 Aerlync Labs Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(soc.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_include_directories(.)

zephyr_compile_definitions_ifdef(CONFIG_SOC_LPC845M301JBD64
    CPU_LPC845M301JBD64
)

# Linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld
    CACHE INTERNAL "")
