/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>

void soc_interrupt_init(void)
{
	(void)arch_irq_lock();

	csr_write(mie, 0);
	csr_write(mip, 0);

#ifdef CONFIG_SMP
	/* For 2nd CPU, we need to enable external interrupt,
	 * as the RISC-V core code only enables MEXT if PLIC
	 * is enabled... and SweRV does not have PLIC.
	 */

	uint32_t mhartid = csr_read(mhartid);

	if (mhartid > 0) {
		irq_enable(RISCV_IRQ_MEXT);
	}
#endif
}

#ifdef CONFIG_PM_CPU_OPS

/** Hart Start Control Register */
#define MHARTSTART 0x7FC

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	ARG_UNUSED(entry_point);

	csr_write(MHARTSTART, BIT(cpuid));

	return 0;
}

#endif /* CONFIG_PM_CPU_OPS */
