/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 * Initialization of full C support: zero the .bss and call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel_internal.h>
#include <linker/linker-defs.h>

extern FUNC_NORETURN void z_cstart(void);

static inline void z_arm64_bss_zero(void)
{
	uint64_t *p = (uint64_t *)__bss_start;
	uint64_t *end = (uint64_t *)__bss_end;

	while (p < end) {
		*p++ = 0;
	}
}

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */
void z_arm64_prep_c(void)
{
	z_arm64_bss_zero();
	z_arm64_interrupt_init();
	z_cstart();

	CODE_UNREACHABLE;
}
