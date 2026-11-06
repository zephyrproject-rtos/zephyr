/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cache.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/barrier.h>
#include <kernel_arch_func.h>

struct arm_m_boot_params {
	uintptr_t sp;
	arch_cpustart_t fn;
	void *arg;
};

BUILD_ASSERT(offsetof(struct arm_m_boot_params, sp) == 0);

volatile struct arm_m_boot_params arm_m_cpu_boot_params;

BUILD_ASSERT(IS_ENABLED(CONFIG_ARMV7_M_ARMV8_M_MAINLINE), "secondary CPUs boot with BASEPRI");

#define CPU_HWID(node_id) DT_REG_ADDR(node_id),

static const uint32_t cpu_hwid[] = {DT_FOREACH_CPU(CPU_HWID)};

BUILD_ASSERT(ARRAY_SIZE(cpu_hwid) >= CONFIG_MP_MAX_NUM_CPUS,
	     "fewer cpu nodes than CONFIG_MP_MAX_NUM_CPUS");

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz, arch_cpustart_t fn, void *arg)
{
	arm_m_cpu_boot_params.sp = (uintptr_t)K_KERNEL_STACK_BUFFER(stack) + sz;
	arm_m_cpu_boot_params.fn = fn;
	arm_m_cpu_boot_params.arg = arg;

	sys_cache_data_flush_range((void *)&arm_m_cpu_boot_params, sizeof(arm_m_cpu_boot_params));
	barrier_dsync_fence_full();

	if (pm_cpu_on(cpu_hwid[cpu_num], (uintptr_t)_vector_start) != 0) {
		printk("Failed to boot secondary CPU %d\n", cpu_num);
		k_panic();
	}
}

FUNC_NORETURN void z_arm_secondary_start(void)
{
	arch_cpustart_t fn = arm_m_cpu_boot_params.fn;
	void *arg = arm_m_cpu_boot_params.arg;

	relocate_vector_table();
#if defined(CONFIG_CPU_HAS_FPU)
	z_arm_floating_point_init();
#endif
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	z_arm_interrupt_init();
#endif
#if defined(CONFIG_ARCH_CACHE)
	arch_cache_init();
#endif
	arch_kernel_init();

	fn(arg);

	CODE_UNREACHABLE;
}
