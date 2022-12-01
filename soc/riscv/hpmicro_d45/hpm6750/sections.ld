/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/linker/linker-tool.h>
#include <zephyr/devicetree.h>

#ifdef CONFIG_XIP
__nor_cfg_option_load_addr__ = DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + 0x400;
__boot_header_load_addr__ = DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + 0x1000;
__app_load_addr__ = DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + 0x3000;
__boot_header_length__ = __boot_header_end__ - __boot_header_start__;
__app_offset__ = __app_load_addr__ - __boot_header_load_addr__;
#endif

SECTIONS
{
#ifdef CONFIG_XIP
	.nor_cfg_option __nor_cfg_option_load_addr__ : {
		KEEP(*(.nor_cfg_option))
	} GROUP_LINK_IN(MCU_XIP)

	.boot_header __boot_header_load_addr__ : {
		__boot_header_start__ = .;
		KEEP(*(.boot_header))
		KEEP(*(.fw_info_table))
		KEEP(*(.dc_info))
		__boot_header_end__ = .;
	} GROUP_LINK_IN(MCU_XIP)
#endif
	.start : {
		. = ALIGN(8);
		KEEP(*(.init.*))
		KEEP(*(.start))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(.noncacheable,,)
	{
		. = ALIGN(8);
		__noncacheable_init_start__ = .;
		KEEP(*(.noncacheable.init))
		__noncacheable_init_end__ = .;
		KEEP(*(.noncacheable))
		__noncacheable_bss_start__ = .;
		KEEP(*(.noncacheable.bss))
		__noncacheable_bss_end__ = .;
		. = ALIGN(8);
	} GROUP_LINK_IN(NONCACHEABLE AT> RAM)
	__noncacheable_data_load_start = LOADADDR(.noncacheable);
	__noncacheable_start__ = ORIGIN(NONCACHEABLE);
	__noncacheable_end__ = ORIGIN(NONCACHEABLE) + LENGTH(NONCACHEABLE);
}
