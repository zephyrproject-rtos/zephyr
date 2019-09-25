/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <kernel_structs.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

FUNC_NORETURN void z_irq_spurious(void *unused)
{
	ulong_t mcause;

	ARG_UNUSED(unused);

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;

	LOG_ERR("Spurious interrupt detected! IRQ: %ld", mcause);
#if defined(CONFIG_RISCV_HAS_PLIC)
	if (mcause == RISCV_MACHINE_EXT_IRQ) {
		LOG_ERR("PLIC interrupt line causing the IRQ: %d",
			riscv_plic_get_irq());
	}
#endif
	z_riscv_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      u32_t flags)
{
	ARG_UNUSED(flags);

	z_isr_install(irq, routine, parameter);
#if defined(CONFIG_RISCV_HAS_PLIC)
	riscv_plic_set_priority(irq, priority);
#else
	ARG_UNUSED(priority);
#endif
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
