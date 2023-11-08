/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel/thread_stack.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm/cortex_a_r/lib_helpers.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include "boot.h"
#include "zephyr/cache.h"
#include "zephyr/kernel/thread_stack.h"
#include "zephyr/toolchain/gcc.h"

#define INV_MPID	UINT32_MAX

#define SGI_SCHED_IPI	0
#define SGI_MMCFG_IPI	1
#define SGI_FPU_IPI	2

K_KERNEL_PINNED_STACK_ARRAY_DECLARE(z_interrupt_stacks,
				   CONFIG_MP_MAX_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

K_KERNEL_STACK_ARRAY_DECLARE(z_arm_fiq_stack,
			    CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARMV7_FIQ_STACK_SIZE);

K_KERNEL_STACK_ARRAY_DECLARE(z_arm_abort_stack,
			    CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARMV7_EXCEPTION_STACK_SIZE);

K_KERNEL_STACK_ARRAY_DECLARE(z_arm_undef_stack,
			    CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARMV7_EXCEPTION_STACK_SIZE);

K_KERNEL_STACK_ARRAY_DECLARE(z_arm_svc_stack,
			    CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARMV7_SVC_STACK_SIZE);

K_KERNEL_STACK_ARRAY_DECLARE(z_arm_sys_stack,
			    CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_ARMV7_SVC_STACK_SIZE);

struct boot_params {
	uint32_t mpid;
	char *irq_sp;
	char *fiq_sp;
	char *abt_sp;
	char *udf_sp;
	char *svc_sp;
	char *sys_sp;
	arch_cpustart_t fn;
	void *arg;
	int cpu_num;
};

/* Offsets used in reset.S */
BUILD_ASSERT(offsetof(struct boot_params, mpid) == BOOT_PARAM_MPID_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, irq_sp) == BOOT_PARAM_IRQ_SP_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, fiq_sp) == BOOT_PARAM_FIQ_SP_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, abt_sp) == BOOT_PARAM_ABT_SP_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, udf_sp) == BOOT_PARAM_UDF_SP_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, svc_sp) == BOOT_PARAM_SVC_SP_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, sys_sp) == BOOT_PARAM_SYS_SP_OFFSET);

volatile struct boot_params arm_cpu_boot_params = {
	.mpid = -1,
	.irq_sp = (char *)(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE),
	.fiq_sp = (char *)(z_arm_fiq_stack + CONFIG_ARMV7_FIQ_STACK_SIZE),
	.abt_sp = (char *)(z_arm_abort_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE),
	.udf_sp = (char *)(z_arm_undef_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE),
	.svc_sp = (char *)(z_arm_svc_stack + CONFIG_ARMV7_SVC_STACK_SIZE),
	.sys_sp = (char *)(z_arm_sys_stack + CONFIG_ARMV7_SYS_STACK_SIZE),
};

static const uint32_t cpu_node_list[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(cpus), DT_REG_ADDR, (,))};

/* cpu_map saves the maping of core id and mpid */
static uint32_t cpu_map[CONFIG_MP_MAX_NUM_CPUS] = {
	[0 ... (CONFIG_MP_MAX_NUM_CPUS - 1)] = INV_MPID
};

#ifdef CONFIG_ARM_MPU
extern void z_arm_mpu_init(void);
extern void z_arm_configure_static_mpu_regions(void);
#elif defined(CONFIG_ARM_AARCH32_MMU)
extern int z_arm_mmu_init(void);
#endif

/* Called from Zephyr initialization */
void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz, arch_cpustart_t fn, void *arg)
{
	int cpu_count, i, j;
	uint32_t cpu_mpid = 0;
	uint32_t master_core_mpid;

	/* Now it is on master core */
	__ASSERT(arch_curr_cpu()->id == 0, "");
	master_core_mpid = MPIDR_TO_CORE(GET_MPIDR());

	cpu_count = ARRAY_SIZE(cpu_node_list);
	__ASSERT(cpu_count == CONFIG_MP_MAX_NUM_CPUS,
		"The count of CPU Cores nodes in dts is not equal to CONFIG_MP_MAX_NUM_CPUS\n");

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

	/* Pass stack address to secondary core */
	arm_cpu_boot_params.irq_sp = Z_KERNEL_STACK_BUFFER(stack) + sz;
	arm_cpu_boot_params.fiq_sp = Z_KERNEL_STACK_BUFFER(z_arm_fiq_stack[cpu_num])
				     + CONFIG_ARMV7_FIQ_STACK_SIZE;
	arm_cpu_boot_params.abt_sp = Z_KERNEL_STACK_BUFFER(z_arm_abort_stack[cpu_num])
				     + CONFIG_ARMV7_EXCEPTION_STACK_SIZE;
	arm_cpu_boot_params.udf_sp = Z_KERNEL_STACK_BUFFER(z_arm_undef_stack[cpu_num])
				     + CONFIG_ARMV7_EXCEPTION_STACK_SIZE;
	arm_cpu_boot_params.svc_sp = Z_KERNEL_STACK_BUFFER(z_arm_svc_stack[cpu_num])
				     + CONFIG_ARMV7_SVC_STACK_SIZE;
	arm_cpu_boot_params.sys_sp = Z_KERNEL_STACK_BUFFER(z_arm_sys_stack[cpu_num])
				     + CONFIG_ARMV7_SYS_STACK_SIZE;

	arm_cpu_boot_params.fn = fn;
	arm_cpu_boot_params.arg = arg;
	arm_cpu_boot_params.cpu_num = cpu_num;

	/* store mpid last as this is our synchronization point */
	arm_cpu_boot_params.mpid = cpu_mpid;

	barrier_dsync_fence_full();
	sys_cache_data_invd_range(
			(void *)&arm_cpu_boot_params,
			sizeof(arm_cpu_boot_params));

	/*! TODO: Support PSCI
	 *  \todo Support PSCI
	 */

	/* Wait secondary cores up, see arch_secondary_cpu_init */
	while (arm_cpu_boot_params.fn) {
		wfe();
	}

	cpu_map[cpu_num] = cpu_mpid;

	printk("Secondary CPU core %d (MPID:%#x) is up\n", cpu_num, cpu_mpid);
}

/* the C entry of secondary cores */
void arch_secondary_cpu_init(void)
{
	int cpu_num = arm_cpu_boot_params.cpu_num;
	arch_cpustart_t fn;
	void *arg;

	__ASSERT(arm_cpu_boot_params.mpid == MPIDR_TO_CORE(GET_MPIDR()), "");

	/* Initialize tpidrro_el0 with our struct _cpu instance address */
	write_tpidruro((uintptr_t)&_kernel.cpus[cpu_num]);

#ifdef CONFIG_ARM_MPU

	/*! TODO: Unify mpu and mmu initialization function
	 *  \todo Unify mpu and mmu initialization function
	 */
	z_arm_mpu_init();
	z_arm_configure_static_mpu_regions();
#elif defined(CONFIG_ARM_AARCH32_MMU)
	z_arm_mmu_init();
#endif

#ifdef CONFIG_SMP
	arm_gic_secondary_init();

	irq_enable(SGI_SCHED_IPI);

	/*! TODO: FPU irq
	 *  \todo FPU irq
	 */
#endif

	fn = arm_cpu_boot_params.fn;
	arg = arm_cpu_boot_params.arg;
	barrier_dsync_fence_full();

	/*
	 * Secondary core clears .fn to announce its presence.
	 * Primary core is polling for this. We no longer own
	 * arm_cpu_boot_params afterwards.
	 */
	arm_cpu_boot_params.fn = NULL;
	barrier_dsync_fence_full();

	sev();

	fn(arg);
}

#ifdef CONFIG_SMP

static void broadcast_ipi(unsigned int ipi)
{
	uint32_t mpidr = MPIDR_TO_CORE(GET_MPIDR());

	/*
	 * Send SGI to all cores except itself
	 */
	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		uint32_t target_mpidr = cpu_map[i];
		uint8_t aff0;

		if (mpidr == target_mpidr || mpidr == INV_MPID) {
			continue;
		}

		aff0 = MPIDR_AFFLVL(target_mpidr, 0);
		gic_raise_sgi(ipi, (uint64_t)target_mpidr, 1 << aff0);
	}
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

int arch_smp_init(void)
{
	cpu_map[0] = MPIDR_TO_CORE(GET_MPIDR());

	/*
	 * SGI0 is use for sched ipi, this might be changed to use Kconfig
	 * option
	 */
	IRQ_CONNECT(SGI_SCHED_IPI, IRQ_DEFAULT_PRIORITY, sched_ipi_handler, NULL, 0);
	irq_enable(SGI_SCHED_IPI);

	return 0;
}

SYS_INIT(arch_smp_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
