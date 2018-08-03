/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#include <logging/sys_log.h>

/*
 * @brief Configure MPU for the thread
 *
 * This function configures per thread memory map reprogramming the MPU.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_thread(struct k_thread *thread)
{
	arc_core_mpu_disable();
#if defined(CONFIG_MPU_STACK_GUARD)
	configure_mpu_stack_guard(thread);
#endif

#if defined(CONFIG_USERSPACE)
	configure_mpu_user_context(thread);
	configure_mpu_mem_domain(thread);
#endif
	arc_core_mpu_enable();
}

#if defined(CONFIG_MPU_STACK_GUARD)
/*
 * @brief Configure MPU stack guard
 *
 * This function configures per thread stack guards reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_stack_guard(struct k_thread *thread)
{
#if defined(CONFIG_USERSPACE)
	if (thread->thread_base.user_options & K_USER) {
		/* the areas before and after the user stack of thread is
		 * kernel only. These area can be used as stack guard.
		 * -----------------------
		 * |  kernel only area   |
		 * |---------------------|
		 * |  user stack         |
		 * |---------------------|
		 * |privilege stack guard|
		 * |---------------------|
		 * |  privilege stack    |
		 * -----------------------
		 */
		arc_core_mpu_configure(THREAD_STACK_GUARD_REGION,
			thread->arch.priv_stack_start - STACK_GUARD_SIZE,
			STACK_GUARD_SIZE);
		return;
	}
#endif
	arc_core_mpu_configure(THREAD_STACK_GUARD_REGION,
			       thread->stack_info.start - STACK_GUARD_SIZE,
			       STACK_GUARD_SIZE);

}
#endif

#if defined(CONFIG_USERSPACE)
/*
 * @brief Configure MPU user context
 *
 * This function configures the thread's user context.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_user_context(struct k_thread *thread)
{
	SYS_LOG_DBG("configure user thread %p's context", thread);
	arc_core_mpu_configure_user_context(thread);
}


/*
 * @brief Configure MPU memory domain
 *
 * This function configures per thread memory domain reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_mem_domain(struct k_thread *thread)
{
	SYS_LOG_DBG("configure thread %p's domain", thread);
	arc_core_mpu_configure_mem_domain(thread->mem_domain_info.mem_domain);
}

int _arch_mem_domain_max_partitions_get(void)
{
	return arc_core_mpu_get_max_domain_partition_regions();
}

/*
 * Reset MPU region for a single memory partition
 */
void _arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				       u32_t  partition_id)
{
	ARG_UNUSED(domain);

	arc_core_mpu_disable();
	arc_core_mpu_mem_partition_remove(partition_id);
	arc_core_mpu_enable();

}

/*
 * Configure MPU memory domain
 */
void _arch_mem_domain_configure(struct k_thread *thread)
{
	configure_mpu_mem_domain(thread);
}

/*
 * Destroy MPU regions for the mem domain
 */
void _arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	ARG_UNUSED(domain);

	arc_core_mpu_disable();
	arc_core_mpu_configure_mem_domain(NULL);
	arc_core_mpu_enable();
}

/*
 * Validate the given buffer is user accessible or not
 */
int _arch_buffer_validate(void *addr, size_t size, int write)
{
	return arc_core_mpu_buffer_validate(addr, size, write);
}

#endif
