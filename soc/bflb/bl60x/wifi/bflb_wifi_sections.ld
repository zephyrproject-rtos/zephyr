/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL602 WiFi MAC HW DMA descriptor pools must live in WIFI_RAM
 * (0x42042000-0x4204C000), matching the vendor SDK's ram_wifi region
 * (`bl602_flash.ld: ram_wifi (wxa) : ORIGIN = 0x42042000`).  The BL60x
 * DTS adds WIFI_RAM as an overlapping region on top of sram0 -- Zephyr's
 * automatic .bss/.data placement stays in sram0's lower portion.
 *
 *   SHAREDRAMIPC -- ipc_shared_env (host/FW mailbox), 4 KB-aligned
 *   SHAREDRAM    -- RX/TX descriptor pools, scan buffers, MIC contexts
 *
 * Zephyr's arch_data_copy() / arch_bss_zero() do not touch custom
 * memory-regions, so soc_early_init_hook() (in soc/bflb/bl60x/soc.c)
 * zeroes WIFI_RAM at boot.  These sections use (NOLOAD) so no flash
 * image is allocated for them.
 */

SECTION_DATA_PROLOGUE(_bl_static_cfg_entry,,)
{
	_ld_bl_static_cfg_entry_start = .;
	KEEP(*(.bl_static_cfg_entry))
	_ld_bl_static_cfg_entry_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(_bflb_wifi_shramipc, (NOLOAD), ALIGN(4096))
{
	_ld_bflb_wifi_shramipc_start = .;
	KEEP(*(SHAREDRAMIPC))
	_ld_bflb_wifi_shramipc_end = .;
} GROUP_LINK_IN(WIFI_RAM)

SECTION_DATA_PROLOGUE(_bflb_wifi_shram, (NOLOAD),)
{
	. = ALIGN(8);
	_ld_bflb_wifi_shram_start = .;
	KEEP(*(SHAREDRAM))
	. = ALIGN(4);
	_ld_bflb_wifi_shram_end = .;
} GROUP_LINK_IN(WIFI_RAM)
