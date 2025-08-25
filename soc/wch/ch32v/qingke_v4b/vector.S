/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

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
	.word	_isr_wrapper

SECTION_FUNC(vectors, __start)
	li 	a0, 0xe
	csrw	mtvec, a0
	j	__initialize
