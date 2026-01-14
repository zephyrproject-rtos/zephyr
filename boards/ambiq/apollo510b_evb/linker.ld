/*
 * SPDX-FileCopyrightText: 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Board-level linker script fragment using Zephyr macros.
 * Defines sections for assets that are loaded in MRAM but copied to
 * XIP RAM or SSRAM at runtime.
 */

#if CONFIG_BOARD_ENABLE_GPU_ASSET_RELOCATION

    . = ALIGN(32);
SECTION_PROLOGUE(gfx_assets, ,SUBALIGN(8))
{
    /* Define start and end symbols for the VMA region */
    __gfx_assets_start = .;
    *(.gfx.assets)
    *(.gfx.assets.*)
    . = ALIGN(32);
    __gfx_assets_end = .;
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(ambiq_psram)) && DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(ambiq_external_ram_region))
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME_TOKEN(DT_CHOSEN(ambiq_external_ram_region)), FLASH)
#else
} GROUP_DATA_LINK_IN(RAM, FLASH)
#endif

/*
 * Define symbols for the LMA and size, which can be used in C code for memcpy.
 * LOADADDR() gets the LMA of the section.
 * SIZEOF() gets the size of the section.
 */
__gfx_assets_load_start = LOADADDR(gfx_assets);
__gfx_assets_size = SIZEOF(gfx_assets);

#endif

#if CONFIG_LV_Z_MEMORY_POOL_CUSTOM_SECTION
SECTION_PROLOGUE(lvgl_heap, (NOLOAD) ,)
{
    *(.lvgl_heap)
    *(.lvgl_heap*)
} GROUP_LINK_IN(DTCM)
#endif
