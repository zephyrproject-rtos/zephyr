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
 */
enum {
#ifdef CONFIG_NOCACHE_MEMORY
	NOCACHE_MEMORY_REGION,
#endif
#ifdef CONFIG_APPLICATION_MEMORY
	THREAD_APP_DATA_REGION,
#endif
#ifdef CONFIG_MPU_STACK_GUARD
	THREAD_STACK_GUARD_REGION,
#endif
#ifdef CONFIG_COVERAGE_GCOV
	THREAD_GCOV_BSS_REGION,
#endif
#ifdef CONFIG_USERSPACE
	THREAD_STACK_REGION,
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
 * @brief configure a set of fixed (static) MPU regions
 *
 * Internal API function to configure a set of static MPU memory regions,
 * within a (background) memory area determined by start and end address.
 * The total number of HW MPU regions to be programmed depends on the MPU
 * architecture.
 *
 * The function shall be invoked once, upon system initialization.
 *
 * @param static_regions[] an array of memory partitions to be programmed
 * @param regions_num the number of regions to be programmed
 * @param background_area_start the start address of the background memory area
 * @param background_area_end the end address of the background memory area
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore:
 * - the number of HW MPU regions to be programmed shall not exceed the number
 *   of available MPU indices,
 * - the size and alignment of the static regions shall comply with the
 *   requirements of the MPU hardware.
 */
void arm_core_mpu_configure_static_mpu_regions(
	const struct k_mem_partition static_regions[], const u8_t regions_num,
	const u32_t background_area_start, const u32_t background_area_end);

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)

/* Number of memory areas, inside which dynamic regions
 * may be programmed in run-time.
 */
#if defined(CONFIG_APPLICATION_MEMORY)
#define MPU_DYNAMIC_REGION_AREAS_NUM 2
#else
#define MPU_DYNAMIC_REGION_AREAS_NUM 1
#endif

/**
 * @brief mark a set of memory regions as eligible for dynamic configuration
 *
 * Internal API function to configure a set of memory regions, determined
 * by their start address and size, as memory areas eligible for dynamically
 * programming MPU regions (such as a supervisor stack overflow guard) at
 * run-time (for example, thread upon context-switch).
 *
 * The function shall be invoked once, upon system initialization.
 *
 * @param dyn_region_areas an array of k_mem_partition objects declaring the
 *                             eligible memory areas for dynamic programming
 * @param dyn_region_areas_num the number of eligible areas for dynamic
 *                             programming.
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore, the requested areas shall correspond to
 * static memory regions, configured earlier by
 * arm_core_mpu_configure_static_mpu_regions().
 */
void arm_core_mpu_mark_areas_for_dynamic_regions(
	const struct k_mem_partition dyn_region_areas[],
	const u8_t dyn_region_areas_num);

#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief configure a set of dynamic MPU regions
 *
 * Internal API function to configure a set of dynamic MPU memory regions
 * within a (background) memory area. The total number of HW MPU regions
 * to be programmed depends on the MPU architecture.
 *
 * @param dynamic_regions[] an array of memory partitions to be programmed
 * @param regions_num the number of regions to be programmed
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore, the number of HW MPU regions to be programmed shall
 * not exceed the number of (currently) available MPU indices.
 */
void arm_core_mpu_configure_dynamic_mpu_regions(
	const struct k_mem_partition dynamic_regions[], u8_t regions_num);

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arm_core_mpu_configure(u8_t type, u32_t base, u32_t size);

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
