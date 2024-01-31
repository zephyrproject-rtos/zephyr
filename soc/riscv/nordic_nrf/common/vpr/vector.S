/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* Imports */
GTEXT(__initialize)

/* Exports */
GTEXT(__start)

SECTION_FUNC(vectors, __start)
	/* Set mtvec.base (mtvec.mode is RO, no need to mask it). */
	la t0, _isr_wrapper
	csrw mtvec, t0

	/* Set mtvt. */
	la t0, _irq_vector_table
	csrw 0x307, t0

	/* Enable mstatus.mie */
	li t0, 0x1888
	csrw mstatus, t0

	/* Call into Zephyr initialization. */
	tail __initialize
