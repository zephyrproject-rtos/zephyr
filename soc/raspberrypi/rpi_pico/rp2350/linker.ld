/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>

#if CONFIG_RP2_REQUIRES_IMAGE_DEFINITION_BLOCK
MEMORY
{
	IMAGE_DEF_FLASH (r) : ORIGIN = 0x10000000, LENGTH = 128
}

SECTIONS
{
	.image_def : {
		LONG(0xffffded3) /* PICOBIN_BLOCK_MARKER_START */
		SHORT(0x0142) /* IMAGE_DEF Item */
#if defined(CONFIG_ARM)
		SHORT(0x1021) /* Executable for Arm */
		LONG(0x00000203) /* VECTOR_TABLE Item */
		LONG(ABSOLUTE(_vector_start)) /* - Address of the vector table in flash */
		SHORT(0x03ff) /* Last Item in Block */
#else
		SHORT(0x1101) /* Executable for RISC-V */
		LONG(0x00000344) /* ENTRY_POINT Item */
		LONG(ABSOLUTE(__start)) /* - Entry point */
		LONG(ABSOLUTE(z_interrupt_stacks)) /* - Initial stack pointer address */
		SHORT(0x04ff) /* Last Item in Block */
#endif
		SHORT(0x0000) /* Padding */
		LONG(0x00000000) /* End of block loop */
		LONG(0xab123579) /* PICOBIN_BLOCK_MARKER_END */
	} > IMAGE_DEF_FLASH
}
#endif /* CONFIG_RP2_REQUIRES_IMAGE_DEFINITION_BLOCK */

#if defined(CONFIG_ARM)
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
#else
#include <zephyr/arch/riscv/common/linker.ld>
#endif
