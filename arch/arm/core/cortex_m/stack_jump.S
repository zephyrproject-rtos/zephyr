/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(arch_stack_jump)

SECTION_FUNC(TEXT, arch_stack_jump)
	mov	sp, r0
	mov	r0, r2
	bx	r1
