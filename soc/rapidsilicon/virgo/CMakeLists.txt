# Copyright (c) 2024 Rapid Silicon
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  start.S
  soc_irq.S
  rapidsi_scu.c
)

zephyr_linker_sources(ROM_START SORT_KEY 0x0 common_linker/init.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_ANDES_V5_EXECIT RODATA SORT_KEY 0x0 common_linker/execit.ld)
zephyr_linker_sources_ifdef(CONFIG_XIP RAM_SECTIONS SORT_KEY 0x0 common_linker/ram_start_nonzero.ld)

# Note: AndeStar V5 EXEC.IT needs custom Andes V5 toolchain
if(CONFIG_SOC_ANDES_V5_EXECIT)
  zephyr_cc_option(-mexecit)
  zephyr_ld_options(-Wl,--mexecit)
endif()

if(CONFIG_SOC_VIRGO)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
endif()
