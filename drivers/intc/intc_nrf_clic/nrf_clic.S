/*
 * Copyright (C) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* Imports */
GDATA(_intl_handlers)

/* Exports */
GTEXT(__soc_handle_irq)
GTEXT(__soc_prep_isr)

/*
 * No need to clear anything, pending bit is cleared by HW.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	ret

/*
 * Call interrupt line handler in _intl_handlers array
 */
SECTION_FUNC(exception.other, __soc_prep_isr)
	/* Subtract reserved interrupt lines from interrupt line number */
	addi a0, a0, -16

	/*
	 * Convert interrupt line number to offset in interrupt line handler array which is 4
	 * bytes wide
	 */
	slli a0, a0, 2

	/* Load base address of interrupt line handler array */
	la t0, _intl_handlers

	/* Add interrupt line offset */
	add t1, t0, a0

	/* Get interrupt line handler */
	lw t1, 0(t1)

	/* Return to caller */
	ret
