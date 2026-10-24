/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/*
 * What the BootROM reads from an image written at flash offset 0. It takes
 * the initial stack pointer and the entry point from the first two words,
 * the way a Cortex-M takes them from a vector table, and it starts the image
 * only if the magic below is present at offset 0x100.
 *
 * The Zephyr vector table follows this block rather than starting the image,
 * because on a Cortex-M the table is contiguous and would otherwise cover
 * offset 0x100 with the entries of interrupts 48 and above. relocate_vector_table()
 * points VTOR at it during early boot.
 *
 * The two fault entries below are the ones the hardware reads while VTOR
 * still addresses this block, which is the window between reset and that
 * relocation. Without them a fault in early boot reads a zero vector and
 * locks up instead of reporting.
 */

GDATA(z_main_stack)

.section .bk_boot_header, "a"
.globl __bk_boot_header
__bk_boot_header:
	.word z_main_stack + CONFIG_MAIN_STACK_SIZE
	.word z_arm_reset
	.word z_arm_nmi
	.word z_arm_hard_fault

	.org 0x100
	.ascii "BK7236"
	.byte 0x10

	.org 0x140
