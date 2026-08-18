/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(arch_stack_jump)

SECTION_FUNC(TEXT, arch_stack_jump)
	mv	sp, a0
	mv	a0, a2
	jr	a1
