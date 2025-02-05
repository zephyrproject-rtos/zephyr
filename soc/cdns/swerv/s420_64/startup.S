/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2026 Intel Corporation
 *
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

#if CONFIG_INCLUDE_RESET_VECTOR
GTEXT(__reset)
#endif

SECTION_FUNC(vectors, __start)

	.option norvc;

	/* Use direct mode for interrupt/exception handling*/
	la t0, _isr_wrapper
        csrw mtvec, t0

	/*
	 * Set MDAC (Default Access Control) CSR for caching
	 * and side effect settings.
	 */
	li t1, 0x01
	csrw 0x7c0, t1
	fence.i

#ifdef CONFIG_INCLUDE_RESET_VECTOR
	/* Jump to __reset */
	tail __reset
#else
	/* Jump to __initialize */
	tail __initialize
#endif

/* no need for exit, just endless loop here.....was: tail atexit */
1:	j 1b
