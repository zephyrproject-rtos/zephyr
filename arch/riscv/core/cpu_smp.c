/*
 * Copyright (c) 2020 Katsuhiro Suzuki <katsuhiro@katsuster.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>

static volatile struct {
	arch_cpustart_t fn;
	void *arg;
} riscv_cpu_cfg[CONFIG_MP_NUM_CPUS];

/*
 * A flag for wake up slave cores.
 * Master sets slave hartid to it and all slaves wait for this flag
 * until it will be equal to hartid of itself. After wake up, slave
 * will set 0 to this flag to notify master core to go next.
 */
volatile uintptr_t riscv_init_flag;
volatile void *riscv_init_sp;

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	riscv_cpu_cfg[cpu_num].fn = fn;
	riscv_cpu_cfg[cpu_num].arg = arg;

	/* Signal to slave core with initial sp. */
	riscv_init_sp = Z_THREAD_STACK_BUFFER(stack) + sz;
	riscv_init_flag = cpu_num;

	/* Wait for slave core */
	while (riscv_init_flag == cpu_num) {
		;
	}
}

/**
 * @brief Prepare to and run C code for slave core
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @param cpu_num  hartid of slave core
 * @return N/A
 */
void z_riscv_slave_start(int cpu_num)
{
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif

#if defined(CONFIG_RISCV_HAS_PLIC)
	irq_enable(RISCV_MACHINE_EXT_IRQ);
#endif

	riscv_cpu_cfg[cpu_num].fn(riscv_cpu_cfg[cpu_num].arg);
}

#ifdef CONFIG_SMP
void z_riscv_sched_ipi(void)
{
	z_sched_ipi();
}
#endif
