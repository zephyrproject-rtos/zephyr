/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

_ASM_FILE_PROLOGUE

/*
 * Switch TTBR0
 */

GTEXT(z_arm64_set_ttbr0)
SECTION_FUNC(TEXT, z_arm64_set_ttbr0)

	/* Switch the TTBR0 */
	msr	ttbr0_el1, x0
	isb

	ret
