/*
 * Copyright (c) 2016,2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/init.h>
#include <obj_core_init.h>


#ifdef CONFIG_OBJ_CORE_SYSTEM
static struct k_obj_type obj_type_cpu;
static struct k_obj_type obj_type_kernel;

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
static struct k_obj_core_stats_desc  cpu_stats_desc = {
	.raw_size = sizeof(struct k_cycle_stats),
	.query_size = sizeof(struct k_thread_runtime_stats),
	.raw   = z_cpu_stats_raw,
	.query = z_cpu_stats_query,
	.reset = NULL,
	.disable = NULL,
	.enable  = NULL,
};

static struct k_obj_core_stats_desc  kernel_stats_desc = {
	.raw_size = sizeof(struct k_cycle_stats) * CONFIG_MP_MAX_NUM_CPUS,
	.query_size = sizeof(struct k_thread_runtime_stats),
	.raw   = z_kernel_stats_raw,
	.query = z_kernel_stats_query,
	.reset = NULL,
	.disable = NULL,
	.enable  = NULL,
};
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */
#endif /* CONFIG_OBJ_CORE_SYSTEM */

struct k_thread z_idle_threads[CONFIG_MP_MAX_NUM_CPUS];

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
 */
K_KERNEL_PINNED_STACK_ARRAY_DEFINE(z_interrupt_stacks,
				   CONFIG_MP_MAX_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

void z_init_cpu(uint8_t cpu_id)
{
	init_idle_thread(cpu_id);
	_kernel.cpus[cpu_id].idle_thread = &z_idle_threads[cpu_id];
	_kernel.cpus[cpu_id].id = cpu_id;
	_kernel.cpus[cpu_id].irq_stack =
		(K_KERNEL_STACK_BUFFER(z_interrupt_stacks[cpu_id]) +
		 K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[cpu_id]));
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	_kernel.cpus[cpu_id].usage = &_kernel.usage[cpu_id];
	_kernel.cpus[cpu_id].usage->track_usage =
		CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE;
#endif

#ifdef CONFIG_PM
	/*
	 * Increment number of CPUs active. The pm subsystem
	 * will keep track of this from here.
	 */
	atomic_inc(&_cpus_active);
#endif

#ifdef CONFIG_OBJ_CORE_SYSTEM
	k_obj_core_init_and_link(K_OBJ_CORE(&_kernel.cpus[cpu_id]), &obj_type_cpu);
#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_core_stats_register(K_OBJ_CORE(&_kernel.cpus[cpu_id]),
				  _kernel.cpus[cpu_id].usage,
				  sizeof(struct k_cycle_stats));
#endif
#endif
}

#ifdef CONFIG_OBJ_CORE_SYSTEM
int init_cpu_obj_core_list(void)
{
	/* Initialize CPU object type */

	z_obj_type_init(&obj_type_cpu, K_OBJ_TYPE_CPU_ID,
			offsetof(struct _cpu, obj_core));

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_type_stats_init(&obj_type_cpu, &cpu_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	return 0;
}

int init_kernel_obj_core_list(void)
{
	/* Initialize kernel object type */

	z_obj_type_init(&obj_type_kernel, K_OBJ_TYPE_KERNEL_ID,
			offsetof(struct z_kernel, obj_core));

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_type_stats_init(&obj_type_kernel, &kernel_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	k_obj_core_init_and_link(K_OBJ_CORE(&_kernel), &obj_type_kernel);
#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
	k_obj_core_stats_register(K_OBJ_CORE(&_kernel), _kernel.usage,
			sizeof(_kernel.usage));
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

	return 0;
}
#endif /* CONFIG_OBJ_CORE_SYSTEM */
