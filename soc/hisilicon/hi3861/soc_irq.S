/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(__soc_handle_irq)

SECTION_FUNC(exception.other, __soc_handle_irq)
	/* Drivers clear level-triggered peripheral IRQs. */
	li t0, 3
	bne a0, t0, 1f
	csrci mip, 8
1:
	ret
