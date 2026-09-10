/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <soc.h>

#ifdef CONFIG_SOC_NEORV32_READ_FREQUENCY_AT_RUNTIME
extern unsigned int z_clock_hw_cycles_per_sec;

void soc_early_init_hook(void)
{
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(sysinfo));

	z_clock_hw_cycles_per_sec = sys_read32(base + NEORV32_SYSINFO_CLK);
}
#endif /* CONFIG_SOC_NEORV32_READ_FREQUENCY_AT_RUNTIME */

#ifdef CONFIG_RISCV_SOC_INTERRUPT_INIT
void soc_interrupt_init(void)
{
	(void)arch_irq_lock();

	csr_write(mie, 0);
}
#endif /* CONFIG_RISCV_SOC_INTERRUPT_INIT */
