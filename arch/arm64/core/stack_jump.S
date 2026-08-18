/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(arch_stack_jump)

SECTION_FUNC(TEXT, arch_stack_jump)
	mov	sp, x0
	mov	x0, x2
	br	x1
