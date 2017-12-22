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
	arc_core_mpu_disable();
	arc_core_mpu_configure(THREAD_STACK_GUARD_REGION,
			       thread->stack_info.start - STACK_GUARD_SIZE,
			       STACK_GUARD_SIZE);
	arc_core_mpu_enable();
}
#endif

#if defined(CONFIG_USERSPACE)
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
	arc_core_mpu_disable();
	arc_core_mpu_configure_mem_domain(thread->mem_domain_info.mem_domain);
	arc_core_mpu_enable();
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
