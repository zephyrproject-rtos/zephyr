/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2026 Anuj Deshpande
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

	/*
	 * Use direct mode for interrupt/exception handling: point mtvec at the
	 * common RISC-V ISR wrapper. The THEJAS32 interrupt controller routes
	 * all peripheral sources through the machine external interrupt.
	 */
	la t0, _isr_wrapper
	csrw mtvec, t0

#if CONFIG_INCLUDE_RESET_VECTOR
	/* Jump to __reset */
	tail __reset
#else
	/* Jump to __initialize */
	tail __initialize
#endif
