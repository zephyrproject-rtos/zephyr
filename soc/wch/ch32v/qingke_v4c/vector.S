/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#ifndef CONFIG_VECTOR_TABLE_SIZE
#error "VECTOR_TABLE_SIZE must be defined"
#endif

/* Exports */
GTEXT(__start)

/* Imports */
GTEXT(__initialize)

SECTION_FUNC(vectors, ivt)
	.option norvc
	/* Jump to 0x08000008, into the main flash zone where j __start is */
	lui	x5, 0x8000
	jr	0x8(x5)
	j       __start
	.rept   CONFIG_VECTOR_TABLE_SIZE
	.word	_isr_wrapper
	.endr

SECTION_FUNC(vectors, __start)
	li 	a0, 0xf
	csrw	mtvec, a0
	j	__initialize
