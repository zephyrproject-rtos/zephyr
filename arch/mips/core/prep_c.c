/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 */

#include <kernel_internal.h>
#include <zephyr/irq.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>

static void interrupt_init(void)
{
	extern char __isr_vec[];
	extern uint32_t mips_cp0_status_int_mask;
	unsigned long ebase;

	irq_lock();

	mips_cp0_status_int_mask = 0;

	ebase = 0x80000000;

	memcpy((void *)(ebase + 0x180), __isr_vec, 0x80);

	/*
	 * Disable boot exception vector in BOOTROM,
	 * use exception vector in RAM.
	 */
	write_c0_status(read_c0_status() & ~(ST0_BEV));
}

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void z_prep_c(void)
{
#if defined(CONFIG_SOC_PREP_HOOK)
	soc_prep_hook();
#endif
	z_bss_zero();

	interrupt_init();
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif

	z_cstart();
	CODE_UNREACHABLE;
}
