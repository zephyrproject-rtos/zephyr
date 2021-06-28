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
#include "boot.h"

#define SGI_SCHED_IPI	0
#define SGI_PTABLE_IPI	1
#define SGI_FPU_IPI	2

struct boot_params {
	uint64_t mpid;
	char *sp;
	arch_cpustart_t fn;
	void *arg;
	int cpu_num;
};

/* Offsets used in reset.S */
BUILD_ASSERT(offsetof(struct boot_params, mpid) == BOOT_PARAM_MPID_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, sp) == BOOT_PARAM_SP_OFFSET);

volatile struct boot_params __aligned(L1_CACHE_BYTES) arm64_cpu_boot_params = {
	.mpid = -1,
};

#define CPU_REG_ID(cpu_node_id) DT_REG_ADDR(cpu_node_id),

static const uint64_t cpu_node_list[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(cpus), CPU_REG_ID)
};

extern void z_arm64_mm_init(bool is_primary_core);

/* Called from Zephyr initialization */
void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	int cpu_count, i, j;
	uint64_t cpu_mpid = 0;
	uint64_t master_core_mpid;

	/* Now it is on master core */
	__ASSERT(arch_curr_cpu()->id == 0, "");
	master_core_mpid = MPIDR_TO_CORE(GET_MPIDR());

	cpu_count = ARRAY_SIZE(cpu_node_list);
	__ASSERT(cpu_count == CONFIG_MP_NUM_CPUS,
		"The count of CPU Cores nodes in dts is not equal to CONFIG_MP_NUM_CPUS\n");

	for (i = 0, j = 0; i < cpu_count; i++) {
		if (cpu_node_list[i] == master_core_mpid) {
			continue;
		}
		if (j == cpu_num - 1) {
			cpu_mpid = cpu_node_list[i];
			break;
		}
		j++;
	}
	if (i == cpu_count) {
		printk("Can't find CPU Core %d from dts and failed to boot it\n", cpu_num);
		return;
	}

	arm64_cpu_boot_params.sp = Z_THREAD_STACK_BUFFER(stack) + sz;
	arm64_cpu_boot_params.fn = fn;
	arm64_cpu_boot_params.arg = arg;
	arm64_cpu_boot_params.cpu_num = cpu_num;

	dsb();

	/* store mpid last as this is our synchronization point */
	arm64_cpu_boot_params.mpid = cpu_mpid;

	arch_dcache_range((void *)&arm64_cpu_boot_params,
			  sizeof(arm64_cpu_boot_params),
			  K_CACHE_WB_INVD);

	if (pm_cpu_on(cpu_mpid, (uint64_t)&__start)) {
		printk("Failed to boot secondary CPU core %d (MPID:%#llx)\n",
		       cpu_num, cpu_mpid);
		return;
	}

	/* Wait secondary cores up, see z_arm64_secondary_start */
	while (arm64_cpu_boot_params.fn) {
		wfe();
	}
	printk("Secondary CPU core %d (MPID:%#llx) is up\n", cpu_num, cpu_mpid);
}

/* the C entry of secondary cores */
void z_arm64_secondary_start(void)
{
	int cpu_num = arm64_cpu_boot_params.cpu_num;
	arch_cpustart_t fn;
	void *arg;

	__ASSERT(arm64_cpu_boot_params.mpid == MPIDR_TO_CORE(GET_MPIDR()), "");

	/* Initialize tpidrro_el0 with our struct _cpu instance address */
	write_tpidrro_el0((uintptr_t)&_kernel.cpus[cpu_num]);

	z_arm64_mm_init(false);

#ifdef CONFIG_SMP
	arm_gic_secondary_init();

	irq_enable(SGI_SCHED_IPI);
#ifdef CONFIG_USERSPACE
	irq_enable(SGI_PTABLE_IPI);
#endif
#ifdef CONFIG_FPU_SHARING
	irq_enable(SGI_FPU_IPI);
#endif
#endif

	fn = arm64_cpu_boot_params.fn;
	arg = arm64_cpu_boot_params.arg;
	dsb();

	/*
	 * Secondary core clears .fn to announce its presence.
	 * Primary core is polling for this. We no longer own
	 * arm64_cpu_boot_params afterwards.
	 */
	arm64_cpu_boot_params.fn = NULL;
	dsb();
	sev();

	fn(arg);
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

#ifdef CONFIG_FPU_SHARING
void flush_fpu_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	disable_irq();
	z_arm64_flush_local_fpu();
	/* no need to re-enable IRQs here */
}

void z_arm64_flush_fpu_ipi(unsigned int cpu)
{
	const uint64_t mpidr = GET_MPIDR();

	gic_raise_sgi(SGI_FPU_IPI, mpidr, (1 << cpu));
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
#ifdef CONFIG_FPU_SHARING
	IRQ_CONNECT(SGI_FPU_IPI, IRQ_DEFAULT_PRIORITY, flush_fpu_ipi_handler, NULL, 0);
	irq_enable(SGI_FPU_IPI);
#endif

	return 0;
}
SYS_INIT(arm64_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
