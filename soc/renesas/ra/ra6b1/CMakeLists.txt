# SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_linker_sources_ifndef(
  CONFIG_BOOTLOADER_MCUBOOT
  ROM_START
  SORT_KEY 0
  product_image_headers.ld
)

zephyr_linker_sources(
  NOINIT
  SORT_KEY 0
  ram_noinit.ld
)

zephyr_linker_sources(
  RWDATA
  SORT_KEY 0
  ram_from_flash.ld
)

zephyr_linker_sources(
  RAM_SECTIONS
  SORT_KEY 0
  ram_zero.ld
  ram_entropy.ld
)

# __ddsc_FLASH_END symbol is needed for caching NVM flash
zephyr_linker_sources(SECTIONS provide_symbols.ld)

zephyr_linker_sources_ifdef(
   CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
   RAMFUNC_SECTION
   ram_code_from_flash.ld
)

zephyr_library()
zephyr_library_sources(soc.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT
    ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

# Copy J-Link script to build folder.
set(ERASE_HEADERS 0)
configure_file(${ZEPHYR_HAL_RENESAS_MODULE_DIR}/zephyr/ra_wireless/scripts/ra6b1.jlinkscript
               ${PROJECT_BINARY_DIR}/ra6b1.jlinkscript @ONLY)
