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
	j       __start
	.rept 38
	.word	_isr_wrapper
	.endr

SECTION_FUNC(vectors, __start)
	li 	a0, 3
	csrw	mtvec, a0
	j	__initialize
