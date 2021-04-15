/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief codes required for AArch64 multicore and Zephyr smp support
 */

#include <cache.h>
#include <device.h>
#include <devicetree.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <soc.h>
#include <init.h>
#include <arch/arm64/arm_mmu.h>
#include <arch/cpu.h>
#include <drivers/interrupt_controller/gic.h>
#include <drivers/pm_cpu_ops.h>
#include <sys/arch_interface.h>

#define SGI_SCHED_IPI	0
#define SGI_PTABLE_IPI	1

volatile arm64_cpu_init_data_t arm64_cpu_init[CONFIG_MP_NUM_CPUS];

extern void __start(void);

#define CPU_REG_ID(cpu_node_id) DT_REG_ADDR(cpu_node_id),

const uint64_t cpu_node_list[] = {
	DT_FOREACH_CHILD(DT_PATH(cpus), CPU_REG_ID)
};

/* Called from Zephyr initialization */
void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	int cpu_count, cpu_mpid, i = 0, j = 0;
	/* Now it is on master Core */
	int master_core_mpid = MPIDR_TO_CORE(GET_MPIDR());

	__ASSERT(sizeof(arm64_cpu_init[0]) == ARM64_CPU_INIT_SIZE,
		 "ARM64_CPU_INIT_SIZE != sizeof(arm64_cpu_init[0]\n");

	cpu_count = ARRAY_SIZE(cpu_node_list);
	__ASSERT(cpu_count == CONFIG_MP_NUM_CPUS,
		"The count of CPU Cores nodes in dts is not equal to CONFIG_MP_NUM_CPUS\n");

	do {
		if (cpu_node_list[i] != master_core_mpid)
			j++;
		if (j == cpu_num) {
			cpu_mpid = cpu_node_list[i];
			break;
		}
		i++;
	} while (i < cpu_count);
	if (i == cpu_count) {
		printk("Can't find CPU Core %d from dts and failed to boot it\n", cpu_num);
		return;
	}

	arm64_cpu_init[cpu_num].mpid = cpu_mpid;
	arm64_cpu_init[cpu_num].fn = fn;
	arm64_cpu_init[cpu_num].arg = arg;
	arm64_cpu_init[cpu_num].sp =
		(void *)(Z_THREAD_STACK_BUFFER(stack) + sz);

	arch_dcache_range((void *)&arm64_cpu_init[cpu_num],
			  sizeof(arm64_cpu_init[cpu_num]), K_CACHE_WB_INVD);

	if (pm_cpu_on(cpu_mpid, (uint64_t)&__start)) {
		printk("Failed to boot Secondary CPU Core%d(MPID:%d)\n", cpu_num, cpu_mpid);
		return;
	}

	/* Wait secondary cores up, see z_arm64_secondary_start */
	while (arm64_cpu_init[cpu_num].fn) {
		wfe();
	}
	printk("Secondary CPU Core%d(MPID:%d) is up\n", cpu_num, cpu_mpid);
}

/* the C entry of secondary cores */
void z_arm64_secondary_start(void)
{
	arch_cpustart_t fn;
	int cpu_mpid = MPIDR_TO_CORE(GET_MPIDR());
	int cpu_num = 0;

	while (cpu_num < CONFIG_MP_NUM_CPUS) {
		if (arm64_cpu_init[cpu_num].mpid == cpu_mpid)
			break;
		cpu_num++;
	};
	__ASSERT(cpu_num < CONFIG_MP_NUM_CPUS,
			"Can't find the core in arm64_cpu_init\n");

	/* Initialize tpidrro_el0 with our struct _cpu instance address */
	write_tpidrro_el0((uintptr_t)&_kernel.cpus[cpu_num]);

	z_arm64_mmu_init(false);

#ifdef CONFIG_SMP
	arm_gic_secondary_init();

	irq_enable(SGI_SCHED_IPI);
#ifdef CONFIG_USERSPACE
	irq_enable(SGI_PTABLE_IPI);
#endif
#endif

	fn = arm64_cpu_init[cpu_num].fn;

	/*
	 * Secondary core clears .fn to announce its presence.
	 * Primary core is polling for this.
	 */
	arm64_cpu_init[cpu_num].fn = NULL;

	dsb();
	sev();

	fn(arm64_cpu_init[cpu_num].arg);
}

#ifdef CONFIG_SMP

static void broadcast_ipi(unsigned int ipi)
{
	const uint64_t mpidr = GET_MPIDR();

	/*
	 * Send SGI to all cores except itself
	 * Note: Assume only one Cluster now.
	 */
	gic_raise_sgi(ipi, mpidr, SGIR_TGT_MASK & ~(1 << MPIDR_TO_CORE(mpidr)));
}

void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	z_sched_ipi();
}

/* arch implementation of sched_ipi */
void arch_sched_ipi(void)
{
	broadcast_ipi(SGI_SCHED_IPI);
}

#ifdef CONFIG_USERSPACE
void ptable_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	/*
	 * Make sure a domain switch by another CPU is effective on this CPU.
	 * This is a no-op if the page table is already the right one.
	 */
	z_arm64_swap_ptables(_current);
}

void z_arm64_ptable_ipi(void)
{
	broadcast_ipi(SGI_PTABLE_IPI);
}
#endif

static int arm64_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * SGI0 is use for sched ipi, this might be changed to use Kconfig
	 * option
	 */
	IRQ_CONNECT(SGI_SCHED_IPI, IRQ_DEFAULT_PRIORITY, sched_ipi_handler, NULL, 0);
	irq_enable(SGI_SCHED_IPI);

#ifdef CONFIG_USERSPACE
	IRQ_CONNECT(SGI_PTABLE_IPI, IRQ_DEFAULT_PRIORITY, ptable_ipi_handler, NULL, 0);
	irq_enable(SGI_PTABLE_IPI);
#endif

	return 0;
}
SYS_INIT(arm64_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
