/*
 * Copyright (c) 2021,2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__reset)

/* imports */
GTEXT(__initialize)

SECTION_FUNC(reset, __reset)
	/* Disable interrupts */
	csrw mstatus, x0
	csrw mie, x0

#ifdef CONFIG_USERSPACE
	/* Disable counter access outside M-mode */
	csrw mcounteren, x0
#endif /* CONFIG_USERSPACE */

	/* Allow mcycle and minstret counters to increment */
	li x11, ~5
	csrw mcountinhibit, x11

	/* Zerorize counters */
	csrw mcycle, x0
	csrw mcycleh, x0
	csrw minstret, x0
	csrw minstreth, x0

	/* Jump to __initialize */
	call __initialize
