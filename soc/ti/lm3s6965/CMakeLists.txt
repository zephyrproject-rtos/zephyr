# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  soc_config.c
  reboot.S
  sys_arch_reboot.c
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/arm/include
  )

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
