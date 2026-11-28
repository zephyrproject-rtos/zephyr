/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief XScale idle mode entry
 *
 * An XScale core waits for an interrupt in idle mode, entered by writing
 * IDLE to the CP14 PWRMODE register. On a PXA270 the core does not come back
 * from idle when that write is the first word of an instruction cache line,
 * so wherever the compiler happened to place an inline write, some builds
 * hung on their first idle. As Linux's cpu_xscale_do_idle does, keep the
 * sequence in a function of its own, aligned on a cache line: the write is
 * then always its second word, and the return its third.
 */

#include <zephyr/toolchain.h>

_ASM_FILE_PROLOGUE

	.section .text.z_xscale_idle, "ax", %progbits
	.balign	32
	.global	z_xscale_idle
	.type	z_xscale_idle, %function
z_xscale_idle:
	mov	r0, #1
	mcr	p14, 0, r0, c7, c0, 0		/* PWRMODE = IDLE */
	bx	lr
	.size	z_xscale_idle, . - z_xscale_idle
