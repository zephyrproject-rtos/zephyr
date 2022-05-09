/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Second Stage Bootloader is only linked to the app that
 * resides at 0x100. This can be the application, or a bootloader
 * such as mcuboot.
 */
#if CONFIG_RP2_REQUIRES_SECOND_STAGE_BOOT
MEMORY
{
	BOOT_FLASH (r) : ORIGIN = 0x10000000, LENGTH = 256
}

SECTIONS
{
	.boot2 : {
		KEEP(*(.boot2))
	} > BOOT_FLASH
}
#endif /* CONFIG_RP2_REQUIRES_SECOND_STAGE_BOOT */

#include <zephyr/arch/arm/aarch32/cortex_m/scripts/linker.ld>
