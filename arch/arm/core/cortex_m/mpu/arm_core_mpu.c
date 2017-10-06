/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>
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
	arm_core_mpu_disable();
	arm_core_mpu_configure(THREAD_STACK_GUARD_REGION,
			thread->stack_info.start - MPU_GUARD_ALIGN_AND_SIZE,
			thread->stack_info.size);
	arm_core_mpu_enable();
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
	arm_core_mpu_disable();
	arm_core_mpu_configure_mem_domain(thread->mem_domain_info.mem_domain);
	arm_core_mpu_enable();
}

int _arch_mem_domain_max_partitions_get(void)
{
	return arm_core_mpu_get_max_domain_partition_regions();
}

/**
 * @brief Reset MPU region for a single memory partition
 *
 * @param   domain  The memory domain structure
 * @param   part_index  memory partition index
 */
void _arch_mem_domain_remove_partition(struct k_mem_domain *domain,
				       u32_t  partition_id)
{
	ARG_UNUSED(domain);

	arm_core_mpu_disable();
	arm_core_mpu_remove_mem_partition(partition_id);
	arm_core_mpu_enable();

}

/**
 * @brief Destroy MPU regions for the mem domain
 *
 * @param   domain  The memory domain structure
 * @param   part_index  memory partition index
 */
void _arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	ARG_UNUSED(domain);

	arm_core_mpu_disable();
	arm_core_mpu_configure_mem_domain(NULL);
	arm_core_mpu_enable();
}

#endif
