/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The defines below represent the region types. The MPU driver is responsible
 * for allocating the region according to the type and for setting the correct
 * attributes.
 *
 * Each MPU is different and has a different set of attributes, hence instead
 * of having the attributes at this level, the arm_mpu_core defines the intent
 * types.
 *
 * An intent type (i.e. THREAD_STACK_GUARD) can correspond to a different set
 * of operations and attributes for each MPU and it is the responsibility of
 * the MPU driver to select the correct ones.
 *
 * The intent based configuration can't fail hence at this level no error
 * is returned by the configuration functions.
 * If one of the operations corresponding to an intent fails the error has to
 * be managed inside the MPU driver and to not be escalated.
 *
 *
 * Some MPU architectures, such as the standard (unmodified) ARMv8-M MPU,
 * require that the active (i.e. enabled) MPU regions do not overlap.
 * (Overlapping MPU regions lead to MemManage Faults.)
 *
 * The requirement for non-overlapping MPU regions is enforced
 * by symbol: CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS. As a result of this
 * requirement, a user-defined MPU region (e.g. of type THREAD_STACK_GUARD)
 * may not be programmed on top of the underlying (existing) SRAM MPU region.
 * An additional "background" SRAM region is, therefore, required to be
 * programmed, for every user-defined MPU region. This "background" region
 * will have the same access permissions as the initially existing SRAM region.
 * In this way we can maintain the same access permissions above and below the
 * user-defined MPU region, and no memory space is left uncovered by the MPU
 * regions. This is shown in the diagram below.
 *
 *
 * Before programming the        After programming the
 * user-defined region:          user-defined region:
 *
 * |.....................|      |.....................|
 * |                     |      |     Additional      |
 * |                     |      |    "Background"     |
 * |                     |      |       SRAM          |
 * |                     |      |      Region         |
 * |                     |      |---------------------|
 * |     Some SRAM       |      |    User-defined     |
 * |      Region         |      |       MPU           |
 * |                     |      |      Region         |
 * |                     |      |---------------------|
 * |                     |      |                     |
 * |                     |      |     Some SRAM       |
 * |                     |      |       Region        |
 * |.....................|      |.....................|
 *
 *
 * Each region type that is dynamically configured during
 * run-time (e.g. THREAD_STACK_REGION) introduces the need
 * for one additional background MPU region.
 *
 * On the contrary, fixed, i.e. non run-time configurable
 * regions (e.g. THREAD_APP_DATA_REGION) do not require
 * any additional background MPU region.
 */
enum {
#ifdef CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS
#ifdef CONFIG_APP_SHARED_MEM
	APP_SHARED_MEM_BKGND_REGION,
#endif /* CONFIG_APP_SHARED_MEM */
	KERNEL_BKGND_REGION,
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
#ifdef CONFIG_APPLICATION_MEMORY
	THREAD_APP_DATA_REGION,
#endif
#ifdef CONFIG_MPU_STACK_GUARD
	THREAD_STACK_GUARD_REGION,
#ifdef CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS
	KERNEL_BKGND_STACK_GUARD_REGION,
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
#endif
#ifdef CONFIG_USERSPACE
	THREAD_STACK_REGION,
#ifdef CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS
	KERNEL_BKGND_THREAD_STACK_REGION,
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
	THREAD_DOMAIN_PARTITION_REGION,
#endif
	THREAD_MPU_REGION_LAST
};

#if defined(CONFIG_ARM_MPU)
struct k_mem_domain;
struct k_mem_partition;
struct k_thread;

/* ARM Core MPU Driver API */

/*
 * This API has to be implemented by all the MPU drivers that have
 * ARM_MPU support.
 */

/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void);

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void);

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arm_core_mpu_configure(u8_t type, u32_t base, u32_t size);

/**
 * @brief reset access permissions for the entire kernel SRAM region
 */
void arm_core_mpu_kernel_ram_region_reset(void);

/**
 * @brief configure MPU regions for the memory partitions of the memory domain
 *
 * @param   mem_domain    memory domain that thread belongs to
 */
void arm_core_mpu_configure_mem_domain(struct k_mem_domain *mem_domain);

/**
 * @brief configure MPU regions for a user thread's context
 *
 * @param	thread	thread to configure
 */
void arm_core_mpu_configure_user_context(struct k_thread *thread);

/**
 * @brief configure MPU region for a single memory partition
 *
 * @param   part_index  memory partition index
 * @param   part        memory partition info
 */
void arm_core_mpu_configure_mem_partition(u32_t part_index,
					  struct k_mem_partition *part);

/**
 * @brief Reset MPU region for a single memory partition
 *
 * @param   part_index  memory partition index
 */
void arm_core_mpu_mem_partition_remove(u32_t part_index);

/**
 * @brief get the maximum number of free regions for memory domain partitions
 */
int arm_core_mpu_get_max_domain_partition_regions(void);

/**
 * @brief validate the given buffer is user accessible or not
 */
int arm_core_mpu_buffer_validate(void *addr, size_t size, int write);

#endif /* CONFIG_ARM_MPU */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_ */
