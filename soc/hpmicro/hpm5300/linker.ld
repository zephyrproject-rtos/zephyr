/*
 * Copyright (c) 2023,2025,2026 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

#define HPM_FLASH_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define HPM_APP_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))
#define HPM_BOOT_HEADER_OFFSET 0x1000
#define HPM_NOR_CFG_OFFSET 0x400
#define HPM_BOOT_REGION_SIZE (HPM_APP_BASE - HPM_FLASH_BASE)

MEMORY
{
	HPM_BOOT (rx) : ORIGIN = HPM_FLASH_BASE, LENGTH = HPM_BOOT_REGION_SIZE
}

SECTIONS
{
	.nor_cfg_option ORIGIN(HPM_BOOT) + HPM_NOR_CFG_OFFSET : {
		KEEP(*(.nor_cfg_option))
	} > HPM_BOOT

#ifdef CONFIG_XIP
	ASSERT(SIZEOF(.nor_cfg_option) == 16, "Missing HPM NOR boot configuration")
#endif

	.boot_header ORIGIN(HPM_BOOT) + HPM_BOOT_HEADER_OFFSET : {
		__boot_header_start__ = .;
		KEEP(*(.boot_header))
		KEEP(*(.fw_info_table))
		__boot_header_end__ = .;
	} > HPM_BOOT
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_NOCACHE_MEMORY
/* HPM5361 has fixed uncached DLM, not programmable PMA regions. */
ASSERT(_nocache_ram_start >= 0x00080000 && _nocache_ram_end <= 0x000a0000,
       "HPM5361 uncached sections must remain in DLM")
#endif

__boot_header_length__ = __boot_header_end__ - __boot_header_start__;
__app_load_addr__ = HPM_APP_BASE;
__app_offset__ = HPM_APP_BASE - (HPM_FLASH_BASE + HPM_BOOT_HEADER_OFFSET);
__fw_size__ = __rom_region_size;
