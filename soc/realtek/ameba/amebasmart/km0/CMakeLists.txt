# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0
#
# KM0 image2 runs from KM0_BD_RAM, loaded by the LP bootloader. Memory regions
# come from the KM0 devicetree chosen nodes.

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../common/linker_cortex_m.ld CACHE INTERNAL "")

# The bootloader reads the entry from Img2EntryFun0 at ORIGIN(RAM). Sort key
# 0x0entry places it after rom_start_offset ("0x0"), before "0x0vectors".
zephyr_linker_sources(ROM_START SORT_KEY 0x0entry km0/image2_entry_section.ld)
zephyr_linker_sources(SECTIONS image2_entry_assert.ld)

# SystemCoreClock for KM0; the SDK ameba_system.c is built only for CA32/KM4.
zephyr_sources(${CMAKE_CURRENT_LIST_DIR}/system_ameba_lp.c)

# DiagVprintf for the HAL logging path (no KM0-ABI chipinfo archive shipped).
zephyr_sources(${CMAKE_CURRENT_LIST_DIR}/rom_patch.c)

zephyr_linker_sources(DATA_SECTIONS SORT_KEY 0x1ipc ipc_table_section.ld)

# KM0 ROM helper addresses (LOGUART_*, DelayUs, ...). With no KM0 chipinfo
# archive, this script is their only provider.
zephyr_link_libraries(
  -T${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/amebasmart/ld/ameba_rom_symbol_km0_acut.ld)
