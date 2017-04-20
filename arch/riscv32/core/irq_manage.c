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
	u32_t mcause;

	ARG_UNUSED(unused);

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;

	printk("Spurious interrupt detected! IRQ: %d\n", (int)mcause);
#if defined(CONFIG_RISCV_HAS_PLIC)
	if (mcause == RISCV_MACHINE_EXT_IRQ) {
		printk("PLIC interrupt line causing the IRQ: %d\n",
		       riscv_plic_get_irq());
	}
#endif

	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
}
