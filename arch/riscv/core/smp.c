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
#include <zephyr/sys/barrier.h>
#if defined(CONFIG_RISCV_IMSIC)
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>
#endif
#include "smp_boot.h"

volatile struct riscv_cpu_boot_slot {
	uintptr_t wake_flag;
	uintptr_t hartid;
	void *sp;
	arch_cpustart_t fn;
	void *arg;
} riscv_cpu_boot_slots[CONFIG_MP_MAX_NUM_CPUS];

BUILD_ASSERT(offsetof(struct riscv_cpu_boot_slot, wake_flag) == RISCV_CPU_BOOT_SLOT_WAKE_FLAG_OFF);
BUILD_ASSERT(offsetof(struct riscv_cpu_boot_slot, hartid) == RISCV_CPU_BOOT_SLOT_HARTID_OFF);
BUILD_ASSERT(offsetof(struct riscv_cpu_boot_slot, sp) == RISCV_CPU_BOOT_SLOT_SP_OFF);
BUILD_ASSERT(offsetof(struct riscv_cpu_boot_slot, fn) == RISCV_CPU_BOOT_SLOT_FN_OFF);
BUILD_ASSERT(offsetof(struct riscv_cpu_boot_slot, arg) == RISCV_CPU_BOOT_SLOT_ARG_OFF);
BUILD_ASSERT(sizeof(struct riscv_cpu_boot_slot) == RISCV_CPU_BOOT_SLOT_SIZE);

#if !defined(CONFIG_PM_CPU_OPS) && (CONFIG_MP_MAX_NUM_CPUS > 1)
/* reset.S tracks "wake flag seen clear" in one bit per slot of a register */
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 8 * sizeof(uintptr_t));
#endif

volatile uintptr_t riscv_cpu_boot_flag;

extern void __start(void);

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	riscv_cpu_boot_slots[cpu_num].wake_flag = 0U;
	riscv_cpu_boot_slots[cpu_num].hartid = _kernel.cpus[cpu_num].arch.hartid;
	riscv_cpu_boot_slots[cpu_num].sp = K_KERNEL_STACK_BUFFER(stack) + sz;
	riscv_cpu_boot_slots[cpu_num].fn = fn;
	riscv_cpu_boot_slots[cpu_num].arg = arg;
	riscv_cpu_boot_flag = 0U;

	/* Release fence: publish the slot stores before the wake flag */
	barrier_dsync_fence_full();
	riscv_cpu_boot_slots[cpu_num].wake_flag = 1U;

#ifdef CONFIG_PM_CPU_OPS
	if (pm_cpu_on(cpu_num, (uintptr_t)&__start)) {
		riscv_cpu_boot_slots[cpu_num].wake_flag = 0U;
		printk("Failed to boot secondary CPU %d\n", cpu_num);
		return;
	}
#endif

	while (riscv_cpu_boot_flag == 0U) {
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
#ifdef CONFIG_CUSTOM_STACK_GUARD
	z_riscv_custom_stack_guard_init();
#endif /* CONFIG_CUSTOM_STACK_GUARD */
#ifdef CONFIG_SMP
	irq_enable(RISCV_IRQ_MSOFT);
#endif /* CONFIG_SMP */
#if defined(CONFIG_PLIC_IRQ_AFFINITY) || defined(CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY)
	/* Enable on secondary cores so that they can respond to PLIC */
	irq_enable(RISCV_IRQ_MEXT);
#endif /* CONFIG_PLIC_IRQ_AFFINITY || CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY */
#if defined(CONFIG_RISCV_IMSIC) && defined(CONFIG_SMP)
	/* Initialize IMSIC on secondary CPU */
	z_riscv_imsic_secondary_init();
#endif /* CONFIG_RISCV_IMSIC && CONFIG_SMP */
	soc_per_core_init_hook();
	riscv_cpu_boot_slots[cpu_num].fn(riscv_cpu_boot_slots[cpu_num].arg);
}
