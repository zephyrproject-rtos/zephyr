/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arc/v2/mpu/arc_core_mpu.h>
#include <zephyr/kernel_structs.h>

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

int arch_mem_domain_max_partitions_get(void)
{
	return arc_core_mpu_get_max_domain_partition_regions();
}

/*
 * Validate the given buffer is user accessible or not
 */
int arch_buffer_validate(void *addr, size_t size, int write)
{
	return arc_core_mpu_buffer_validate(addr, size, write);
}

#endif
