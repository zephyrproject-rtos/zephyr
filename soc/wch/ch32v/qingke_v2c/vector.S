/*
 * Copyright (c) 2024 Michael Hope
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
	lui     a0, %hi(__start)
	jr      %lo(__start)(a0)
	.rept   CONFIG_VECTOR_TABLE_SIZE
	.word	_isr_wrapper
	.endr

SECTION_FUNC(vectors, __start)
	la      a0, ivt
	ori     a0, a0, 3
	csrw	mtvec, a0
	tail	__initialize
