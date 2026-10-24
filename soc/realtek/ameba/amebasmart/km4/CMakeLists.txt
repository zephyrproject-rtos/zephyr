# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0
#
# KM4 non-secure image2. This file is include()d, so CMAKE_CURRENT_SOURCE_DIR
# is the amebasmart directory; use CMAKE_CURRENT_LIST_DIR for local paths.

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../common/linker_cortex_m.ld CACHE INTERNAL "")

zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR})

# KM4 non-secure ROM API addresses (LOGUART_*, DelayUs, FLASH_*, ...). The HAL
# links this script only for MCUboot, so the application adds it here.
zephyr_link_libraries(
  -T${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/amebasmart/ld/ameba_rom_symbol_acut.ld)

# IMG1 reads the entry from Img2EntryFun0 at ORIGIN(RAM). Sort key 0x0entry
# places it after rom_start_offset ("0x0"), before "0x0vectors".
zephyr_linker_sources(ROM_START SORT_KEY 0x0entry km4/image2_entry_section.ld)
zephyr_linker_sources(SECTIONS image2_entry_assert.ld)

zephyr_linker_sources(RAM_SECTIONS SORT_KEY 0x0 km4/app_ram_section.ld)

zephyr_linker_sources(DATA_SECTIONS SORT_KEY 0x1ipc ipc_table_section.ld)

# Without a home, every lib_coex.a .COEXTRACE section is an orphan warning.
zephyr_linker_sources(RODATA SORT_KEY 0x1coex km4/coex_trace_section.ld)

zephyr_sources(${CMAKE_CURRENT_LIST_DIR}/soc.c)
