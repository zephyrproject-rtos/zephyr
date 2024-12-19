/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

MEMORY
{
	IMAGE_DEF_FLASH (r) : ORIGIN = 0x10000000, LENGTH = 128
}

SECTIONS
{
	.image_def : {
		LONG(0xffffded3) /* PICOBIN_BLOCK_MARKER_START */
		LONG(0x10210142) /* IMAGE_DEF Item */
		LONG(0x00000203) /* VECTOR_TABLE Item */
		LONG(ABSOLUTE(_vector_start)) /* - Address of the vector table in flash */
		LONG(0x000003ff) /* Last Item in Block */
		LONG(0x00000000) /* End of block loop */
		LONG(0xab123579) /* PICOBIN_BLOCK_MARKER_END */
	} > IMAGE_DEF_FLASH
}

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
