/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_

#include <zephyr/types.h>
#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ARM_MPU)
struct k_thread;

#if defined(CONFIG_USERSPACE)

/**
 * @brief Maximum number of memory domain partitions
 *
 * This internal macro returns the maximum number of memory partitions, which
 * may be defined in a memory domain, given the amount of available HW MPU
 * regions.
 *
 * @param mpu_regions_num the number of available HW MPU regions.
 */
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
	defined(CONFIG_MPU_GAP_FILLING)
/*
 * For ARM MPU architectures, where the domain partitions cannot be defined
 * on top of the statically configured memory regions, the maximum number of
 * memory domain partitions is set to half of the number of available MPU
 * regions. This ensures that in the worst-case where there are gaps between
 * the memory partitions of the domain, the desired memory map can still be
 * programmed using the available number of HW MPU regions.
 */
#define ARM_CORE_MPU_MAX_DOMAIN_PARTITIONS_GET(mpu_regions_num) \
	(mpu_regions_num/2)
#else
/*
 * For ARM MPU architectures, where the domain partitions can be defined
 * on top of the statically configured memory regions, the maximum number
 * of memory domain partitions is equal to the number of available MPU regions.
 */
#define ARM_CORE_MPU_MAX_DOMAIN_PARTITIONS_GET(mpu_regions_num) \
	(mpu_regions_num)
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief Maximum number of MPU regions required to configure a
 *        memory region for (user) Thread Stack.
 */
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
	defined(CONFIG_MPU_GAP_FILLING)
/* When dynamic regions may not be defined on top of statically
 * allocated memory regions, defining a region for a thread stack
 * requires two additional MPU regions to be configured; one for
 * defining the thread stack and an additional one for partitioning
 * the underlying memory area.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_THREAD_STACK 2
#else
/* When dynamic regions may be defined on top of statically allocated
 * memory regions, a thread stack area may be configured using a
 * single MPU region.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_THREAD_STACK 1
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief Maximum number of MPU regions required to configure a
 *        memory region for a (supervisor) Thread Stack Guard.
 */
#if (defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
		defined(CONFIG_MPU_GAP_FILLING)) \
	|| defined(CONFIG_CPU_HAS_NXP_MPU)
/*
 * When dynamic regions may not be defined on top of statically
 * allocated memory regions, defining a region for a supervisor
 * thread stack guard requires two additional MPU regions to be
 * configured; one for defining the stack guard and an additional
 * one for partitioning the underlying memory area.
 *
 * The same is required for the NXP MPU due to its OR-based decision
 * policy; the MPU stack guard applies more restrictive permissions on
 * the underlying (SRAM) regions, and, therefore, we need to partition
 * the underlying SRAM region.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_MPU_STACK_GUARD 2
#elif defined(CONFIG_CPU_HAS_ARM_MPU)
/* When dynamic regions may be defined on top of statically allocated
 * memory regions, a supervisor thread stack guard area may be configured
 * using a single MPU region.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_MPU_STACK_GUARD 1
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS || CPU_HAS_NXP_MPU */

#endif /* CONFIG_USERSPACE */


/* ARM Core MPU Driver API */

/*
 * This API has to be implemented by all the MPU drivers that have
 * ARM_MPU support.
 */

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
 * @param static_regions an array of pointers to memory partitions
 *                       to be programmed
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
	const struct z_arm_mpu_partition *static_regions,
	const uint8_t regions_num, const uint32_t background_area_start,
	const uint32_t background_area_end);

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)

/* Number of memory areas, inside which dynamic regions
 * may be programmed in run-time.
 */
#define MPU_DYNAMIC_REGION_AREAS_NUM 1

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
 * @param dyn_region_areas an array of z_arm_mpu_partition objects declaring the
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
	const struct z_arm_mpu_partition *dyn_region_areas,
	const uint8_t dyn_region_areas_num);

#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief configure a set of dynamic MPU regions
 *
 * Internal API function to configure a set of dynamic MPU memory regions
 * within a (background) memory area. The total number of HW MPU regions
 * to be programmed depends on the MPU architecture.
 *
 * @param dynamic_regions an array of pointers to memory partitions
 *                        to be programmed
 * @param regions_num the number of regions to be programmed
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore, the number of HW MPU regions to be programmed shall
 * not exceed the number of (currently) available MPU indices.
 */
void arm_core_mpu_configure_dynamic_mpu_regions(
	const struct z_arm_mpu_partition *dynamic_regions,
	uint8_t regions_num);

#if defined(CONFIG_USERSPACE)
/**
 * @brief update configuration of an active memory partition
 *
 * Internal API function to re-configure the access permissions of an
 * active memory partition, i.e. a partition that has earlier been
 * configured in the (current) thread context.
 *
 * @param partition Pointer to a structure holding the partition information
 *                  (must be valid).
 * @param new_attr  New access permissions attribute for the partition.
 *
 * The function shall assert if the operation cannot be not performed
 * successfully (e.g. the given partition can not be found).
 */
void arm_core_mpu_mem_partition_config_update(
	struct z_arm_mpu_partition *partition,
	k_mem_partition_attr_t *new_attr);

#endif /* CONFIG_USERSPACE */

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arm_core_mpu_configure(uint8_t type, uint32_t base, uint32_t size);

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
void arm_core_mpu_configure_mem_partition(uint32_t part_index,
					  struct z_arm_mpu_partition *part);

/**
 * @brief Reset MPU region for a single memory partition
 *
 * @param   part_index  memory partition index
 */
void arm_core_mpu_mem_partition_remove(uint32_t part_index);

/**
 * @brief Get the maximum number of available (free) MPU region indices
 *        for configuring dynamic MPU regions.
 */
int arm_core_mpu_get_max_available_dyn_regions(void);

/**
 * @brief validate the given buffer is user accessible or not
 *
 * Note: Validation will always return failure, if the supplied buffer
 *       spans multiple enabled MPU regions (even if these regions all
 *       permit user access).
 */
int arm_core_mpu_buffer_validate(void *addr, size_t size, int write);

#endif /* CONFIG_ARM_MPU */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_ */
