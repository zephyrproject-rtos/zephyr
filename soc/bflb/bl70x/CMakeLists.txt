# Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

zephyr_code_relocate_ifdef(CONFIG_UART_BFLB FILES ${ZEPHYR_BASE}/drivers/serial/uart_bflb.c
  LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_RISCV_MACHINE_TIMER LIBRARY drivers__timer LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_PINCTRL_BFLB LIBRARY drivers__pinctrl LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_SYSCON_BFLB_EFUSE LIBRARY drivers__syscon LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_CLOCK_CONTROL_BOUFFALOLAB_BL70X_L
  LIBRARY drivers__clock_control LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_CACHE_BFLB_L1C LIBRARY drivers__cache LOCATION ITCM NOKEEP)
zephyr_code_relocate_ifdef(CONFIG_SOC_FLASH_BFLB FILES ${ZEPHYR_BASE}/drivers/flash/flash_bflb.c
  LOCATION ITCM NOKEEP)

# The BLE blob's bflb_rwdata.ld defines __global_pointer$ which causes the
# linker to relax address loads into GP-relative instructions. Since the
# Zephyr CRT does not initialize GP (CONFIG_RISCV_GP is not enabled),
# these relaxed instructions would crash. Disable linker relaxation so
# only the blob's own pre-compiled GP-relative code uses GP.
zephyr_ld_options(-Wl,--no-relax)

zephyr_linker_sources(RWDATA bflb_rwdata.ld)
zephyr_linker_sources(RODATA bflb_rodata.ld)
zephyr_linker_sources(NOINIT bflb_noinit.ld)

# ROM function addresses (PROVIDE symbols) — our shims override these
zephyr_ld_options(-T${CMAKE_CURRENT_SOURCE_DIR}/bflb_rom_symbols.ld)

if(CONFIG_BT_BFLB_BL70X)
  zephyr_sources(
    bflb_platform_shim.c
    rf_callbacks.c
  )
endif()
