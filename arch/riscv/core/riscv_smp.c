/*
 * Copyright (c) 2020 Andes Technology Corporation
 * Copyright (c) 2020 Jim Shu <cwshu@andestech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SMP) || (CONFIG_MP_NUM_CPUS > 1)

#include <kernel.h>
#include <kernel_structs.h>

uint32_t cpu_init_boot_flag[CONFIG_MP_NUM_CPUS] = {0};
ulong_t cpu_init_stack[CONFIG_MP_NUM_CPUS];

volatile struct cpu_init_t {
	int stack_size;
	arch_cpustart_t fn;
	void *arg;
} cpu_init[CONFIG_MP_NUM_CPUS] = {0};

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
                    arch_cpustart_t fn, void *arg)
{
	cpu_init_stack[cpu_num] = (ulong_t)stack + sz;

	cpu_init[cpu_num].stack_size = sz;
	cpu_init[cpu_num].fn = fn;
	cpu_init[cpu_num].arg = arg;

	cpu_init_boot_flag[cpu_num] = 1;
}

/* the C entry of slave cores */
void slave_core_cstart(int cpu_num)
{
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	/* Init mie/mip CSRs (per-CPU) */
	soc_interrupt_init();
#endif

#if defined(CONFIG_RISCV_HAS_PLIC)
	/* Enable machine external IRQ for PLIC driver
	 *
	 * Note: MEXT IRQ of mhart 0 is enabled in the device init of PLIC driver.
	 * MEXT of other mharts should be enabled here instead of device init
	 * because CSRs can only be accessed by mhart itself.
	 */
	irq_enable(RISCV_MACHINE_EXT_IRQ);
#endif

#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	/* Enable machine software IRQ to receive IPI */
	irq_enable(RISCV_MACHINE_SOFT_IRQ);
#endif

	cpu_init[cpu_num].fn(cpu_init[cpu_num].arg);
}

#endif /* defined(CONFIG_SMP) || (CONFIG_MP_NUM_CPUS > 1) */
