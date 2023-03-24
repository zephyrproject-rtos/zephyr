/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_irq_spurious(const void *unused)
{
	uint32_t tbr;

	ARG_UNUSED(unused);

	__asm__ volatile (
		"rd %%tbr, %0" :
		"=r" (tbr)
		);
	LOG_ERR("Spurious interrupt detected! IRQ: %d", (tbr >> 4) & 0xf);
	z_sparc_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void z_sparc_enter_irq(uint32_t irl)
{
	const struct _isr_table_entry *ite;

	_current_cpu->nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	if (irl != 141U) {
		irl = z_sparc_int_get_source(irl);
		ite = &_sw_isr_table[irl];
		ite->isr(ite->arg);
	} else {
		z_irq_do_offload();
	}
#else
	/* Get the actual interrupt source from the interrupt controller */
	irl = z_sparc_int_get_source(irl);
	ite = &_sw_isr_table[irl];
	ite->isr(ite->arg);
#endif

	_current_cpu->nested--;
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif
}
