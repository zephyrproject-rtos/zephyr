/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

SECTION_FUNC(vectors, __start)
#ifdef CONFIG_RISCV_GP
	.option push
	.option norelax
	/* Configure the GP register */
	la gp, __global_pointer$
	.option pop
#endif

	.option norvc;
	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	la t0, _isr_wrapper
	csrw mtvec, t0
	csrwi mie, 0
	/* Jump to __initialize */
	tail __initialize

/*
 * eflash signature used to enable specific function after power-on reset.
 * (HW mechanism)
 * The content of 16-bytes must be the following and at offset 0x40 of binary.
 * ----------------------------------------------------------------------------
 * 1st 2nd 3rd 4th 5th 6th 7th    8th    9th 10th 11th 12th 13th 14th 15th 16th
 * ----------------------------------------------------------------------------
 * A5h A5h A5h A5h A5h A5h [host] [flag] 85h  12h  5Ah  5Ah  AAh  AAh  55h  55h
 * ----------------------------------------------------------------------------
 * 7th [host]: A4h = enable eSPI, A5h = enable LPC
 */
.org 0x40
.balign 16
.global eflash_sig
eflash_sig:
.byte 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5
/* Enable eSPI */
.byte 0xA4
/* Flag of signature. Enable internal clock generator */
.byte 0xB4
.byte 0x85, 0x12, 0x5A, 0x5A, 0xAA, 0xAA, 0x55, 0x55
