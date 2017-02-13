/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <kernel_structs.h>
#include <misc/printk.h>

void _irq_spurious(void *unused)
{
	uint32_t mcause;

	ARG_UNUSED(unused);

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;

	printk("Spurious interrupt detected! IRQ: %d\n", (int)mcause);

	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
}
