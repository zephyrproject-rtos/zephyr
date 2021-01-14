/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc/memory.h>

	.global	_start
	.equ    _start, SOF_TEXT_BASE

#ifndef CONFIG_RESET_VECTOR_IN_BOOTLOADER
	.begin	literal_prefix	.ResetVector
	.section		.ResetVector.text, "ax"

	.literal_position

	.align	4
	.global	__start

__start:
	movi	a0, 0
	call0	_start		 /* jump to _start (in crt1-*.S) */

	/* never returns */

	.size	__start, . - __start

	.end	literal_prefix
#endif
