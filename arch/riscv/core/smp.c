/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/platform/hooks.h>

volatile struct {
	arch_cpustart_t fn;
	void *arg;
} riscv_cpu_init[CONFIG_MP_MAX_NUM_CPUS];

volatile uintptr_t __noinit riscv_cpu_wake_flag;
volatile uintptr_t riscv_cpu_boot_flag;
volatile void *riscv_cpu_sp;

extern void __start(void);

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	riscv_cpu_init[cpu_num].fn = fn;
	riscv_cpu_init[cpu_num].arg = arg;

	riscv_cpu_sp = K_KERNEL_STACK_BUFFER(stack) + sz;
	riscv_cpu_boot_flag = 0U;

#ifdef CONFIG_PM_CPU_OPS
	if (pm_cpu_on(cpu_num, (uintptr_t)&__start)) {
		printk("Failed to boot secondary CPU %d\n", cpu_num);
		return;
	}
#endif

	while (riscv_cpu_boot_flag == 0U) {
		riscv_cpu_wake_flag = _kernel.cpus[cpu_num].arch.hartid;
	}
}

void arch_secondary_cpu_init(int hartid)
{
	unsigned int i;
	unsigned int cpu_num = 0;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (_kernel.cpus[i].arch.hartid == hartid) {
			cpu_num = i;
		}
	}
	csr_write(mscratch, &_kernel.cpus[cpu_num]);
#ifdef CONFIG_SMP
	_kernel.cpus[cpu_num].arch.online = true;
#endif
#if defined(CONFIG_MULTITHREADING) && defined(CONFIG_THREAD_LOCAL_STORAGE)
	__asm__("mv tp, %0" : : "r" (z_idle_threads[cpu_num].tls));
#endif
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
#ifdef CONFIG_RISCV_PMP
	z_riscv_pmp_init();
#endif
#ifdef CONFIG_SMP
	irq_enable(RISCV_IRQ_MSOFT);
#endif /* CONFIG_SMP */
#ifdef CONFIG_PLIC_IRQ_AFFINITY
	/* Enable on secondary cores so that they can respond to PLIC */
	irq_enable(RISCV_IRQ_MEXT);
#endif /* CONFIG_PLIC_IRQ_AFFINITY */
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
	riscv_cpu_init[cpu_num].fn(riscv_cpu_init[cpu_num].arg);
}
