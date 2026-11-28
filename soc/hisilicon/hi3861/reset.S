/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <soc.h>

GTEXT(__reset)
GTEXT(__initialize)

SECTION_FUNC(reset, __reset)
	csrci mstatus, 8
	csrwi mie, 0
	csrwi HI3861_CSR_LOCIE0, 0

	/* Clear ROM cache and PMP state before entering Zephyr. */
	csrwi HI3861_CSR_CACHE_CONTROL0, 0
	csrwi HI3861_CSR_CACHE_CONTROL1, 0
	fence
	csrwi pmpcfg0, 0
	csrwi pmpcfg1, 0

	tail __initialize
