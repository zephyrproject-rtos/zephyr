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
	arc_core_mpu_configure_thread(thread);
	arc_core_mpu_enable();
}

#if defined(CONFIG_USERSPACE)

int z_arch_mem_domain_max_partitions_get(void)
{
	return arc_core_mpu_get_max_domain_partition_regions();
}

/*
 * Reset MPU region for a single memory partition
 */
void z_arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				       u32_t partition_id)
{
	arc_core_mpu_disable();
	arc_core_mpu_remove_mem_partition(domain, partition_id);
	arc_core_mpu_enable();
}

/*
 * Configure MPU memory domain
 */
void z_arch_mem_domain_configure(struct k_thread *thread)
{
	arc_core_mpu_disable();
	arc_core_mpu_configure_mem_domain(thread);
	arc_core_mpu_enable();
}

/*
 * Destroy MPU regions for the mem domain
 */
void z_arch_mem_domain_destroy(struct k_mem_domain *domain)
{
	arc_core_mpu_disable();
	arc_core_mpu_remove_mem_domain(domain);
	arc_core_mpu_enable();
}

void z_arch_mem_domain_partition_add(struct k_mem_domain *domain,
				    u32_t partition_id)
{
	/* No-op on this architecture */
}

/*
 * Validate the given buffer is user accessible or not
 */
int z_arch_buffer_validate(void *addr, size_t size, int write)
{
	return arc_core_mpu_buffer_validate(addr, size, write);
}

#endif
