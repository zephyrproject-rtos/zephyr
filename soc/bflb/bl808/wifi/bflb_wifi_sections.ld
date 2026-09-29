/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
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
	KEEP(*(.fw.SHRAMIPC))
	_ld_bflb_wifi_shramipc_end = .;
} GROUP_LINK_IN(WIFI_RAM)

SECTION_DATA_PROLOGUE(_bflb_wifi_shram, (NOLOAD),)
{
	. = ALIGN(8);
	_ld_bflb_wifi_shram_start = .;
	KEEP(*(.fw.SHRAM))
	. = ALIGN(4);
	_ld_bflb_wifi_shram_end = .;
} GROUP_LINK_IN(WIFI_RAM)
