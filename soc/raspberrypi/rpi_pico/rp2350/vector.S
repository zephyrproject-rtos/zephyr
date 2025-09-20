/*
 * Copyright (c) 2025 Andrew Featherstone <andrew.featherstone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(_isr_wrapper)

SECTION_FUNC(vectors, __start)
	.option norvc;

	call soc_reset_hook

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	la t0, _isr_wrapper
	csrw mtvec, t0

	/* Jump to __reset */
	tail __reset
