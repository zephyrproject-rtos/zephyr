/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>

#ifdef CONFIG_SMP

u32_t __init_riscv_smp_mscratch[CONFIG_MP_NUM_CPUS];
u32_t __init_riscv_smp_go[CONFIG_MP_NUM_CPUS];
u32_t __init_riscv_smp_stacks[CONFIG_MP_NUM_CPUS];
u32_t __init_riscv_smp_entry[CONFIG_MP_NUM_CPUS];
u32_t __init_riscv_smp_keys[CONFIG_MP_NUM_CPUS];
u32_t __init_riscv_smp_start_flags[CONFIG_MP_NUM_CPUS];

void _arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
					    void (*fn)(int, void *), void *arg)
{
	/* Set up data */
	__init_riscv_smp_mscratch[cpu_num] = (u32_t) &(_kernel.cpus[cpu_num]);
	__init_riscv_smp_stacks[cpu_num] = (u32_t) stack + sz;
	__init_riscv_smp_entry[cpu_num] = (u32_t) fn;
	__init_riscv_smp_keys[cpu_num] = (u32_t) 0; /* TODO */
	__init_riscv_smp_start_flags[cpu_num] = (u32_t) arg;

	/* Push the go button */
	__init_riscv_smp_go[cpu_num] = 1;
}

#endif /* CONFIG_SMP */

