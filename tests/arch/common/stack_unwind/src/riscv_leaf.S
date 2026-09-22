/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(leaf_fault)
SECTION_FUNC(TEXT, leaf_fault)

	/*
	 * Model a leaf frame that saves its frame pointer but not ra. The
	 * exception frame is the only source for the direct caller address.
	 */
	addi sp, sp, -16
#ifdef CONFIG_64BIT
	sd s0, 8(sp)
#else
	sw s0, 12(sp)
#endif
	addi s0, sp, 16

	.word 0
