/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ipi_impl.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <ipi.h>
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

#ifdef CONFIG_SMP

static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_SCHED	0
#define IPI_FPU_FLUSH	1

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if ((i != id) && _kernel.cpus[i].arch.online &&
		    ((cpu_bitmap & BIT(i)) != 0)) {
			atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
			z_riscv_ipi_send(i);
		}
	}

	arch_irq_unlock(key);
}

void arch_sched_broadcast_ipi(void)
{
	arch_sched_directed_ipi(IPI_ALL_CPUS_MASK);
}

#ifdef CONFIG_FPU_SHARING
void arch_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	z_riscv_ipi_send(cpu);
}
#endif

void z_riscv_sched_ipi_handler(unsigned int cpu_id)
{
	z_riscv_ipi_clear(cpu_id);

	atomic_val_t pending_ipi = atomic_clear(&cpu_pending_ipi[cpu_id]);

	if (pending_ipi & ATOMIC_MASK(IPI_SCHED)) {
		z_sched_ipi();
	}
#ifdef CONFIG_FPU_SHARING
	if (pending_ipi & ATOMIC_MASK(IPI_FPU_FLUSH)) {
		/* disable IRQs */
		csr_clear(mstatus, MSTATUS_IEN);
		/* perform the flush */
		arch_flush_local_fpu();
		/*
		 * No need to re-enable IRQs here as long as
		 * this remains the last case.
		 */
	}
#endif
}

#ifdef CONFIG_FPU_SHARING
/*
 * Make sure there is no pending FPU flush request for this CPU while
 * waiting for a contended spinlock to become available. This prevents
 * a deadlock when the lock we need is already taken by another CPU
 * that also wants its FPU content to be reinstated while such content
 * is still live in this CPU's FPU.
 */
void arch_spin_relax(void)
{
	atomic_val_t *pending_ipi = &cpu_pending_ipi[_current_cpu->id];

	if (atomic_test_and_clear_bit(pending_ipi, IPI_FPU_FLUSH)) {
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly.
		 */
		arch_float_disable(_current_cpu->arch.fpu_owner);
	}
}
#endif

#endif /* CONFIG_SMP */
