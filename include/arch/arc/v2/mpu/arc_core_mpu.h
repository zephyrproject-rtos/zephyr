/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_MPU_ARC_CORE_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_MPU_ARC_CORE_MPU_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The defines below represent the region types. The MPU driver is responsible
 * to allocate the region accordingly to the type and set the correct
 * attributes.
 *
 * Each MPU is different and has a different set of attributes, hence instead
 * of having the attributes at this level the arc_mpu_core defines the intent
 * types.
 * An intent type (i.e. THREAD_STACK_GUARD) can correspond to a different set
 * of operations and attributes for each MPU and it is responsibility of the
 * MPU driver to select the correct ones.
 *
 * The intent based configuration can't fail hence at this level no error
 * is returned by the configuration functions.
 * If one of the operations corresponding to an intent fails the error has to
 * be managed inside the MPU driver and not escalated.
 */
/* Thread Region Intent Type */
#define THREAD_STACK_USER_REGION 0x0
#define THREAD_STACK_REGION 0x1
#define THREAD_APP_DATA_REGION 0x2
#define THREAD_STACK_GUARD_REGION 0x3
#define THREAD_DOMAIN_PARTITION_REGION 0x4

#if defined(CONFIG_ARC_CORE_MPU)
/* ARC Core MPU Driver API */

/*
 * This API has to be implemented by all the MPU drivers that have
 * ARC_CORE_MPU support.
 */

/**
 * @brief enable the MPU
 */
void arc_core_mpu_enable(void);

/**
 * @brief disable the MPU
 */
void arc_core_mpu_disable(void);

/*
 * Before configure the MPU regions, MPU should be disabled
 */
/**
 * @brief configure the default region
 *
 * @param   region_attr region attribute of default region
 */
void arc_core_mpu_default(u32_t region_attr);

/**
 * @brief configure the MPU region
 *
 * @param   index   MPU region index
 * @param   base    base address
 * @param   size    size of region
 * @param   region_attr region attribute
 */
void arc_core_mpu_region(u32_t index, u32_t base, u32_t size,
			 u32_t region_attr);

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arc_core_mpu_configure(u8_t type, u32_t base, u32_t size);
#endif /* CONFIG_ARC_CORE_MPU */


#if defined(CONFIG_MPU_STACK_GUARD)
/**
 * @brief Configure MPU stack guard
 *
 * This function configures per thread stack guards reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_stack_guard(struct k_thread *thread);
#endif

#if defined(CONFIG_USERSPACE)
void arc_core_mpu_configure_user_context(struct k_thread *thread);
void arc_core_mpu_configure_mem_domain(struct k_mem_domain *mem_domain);
void arc_core_mpu_mem_partition_remove(u32_t part_index);
void arc_core_mpu_configure_mem_partition(u32_t part_index,
					  struct k_mem_partition *part);
int arc_core_mpu_get_max_domain_partition_regions(void);
int arc_core_mpu_buffer_validate(void *addr, size_t size, int write);

/*
 * @brief Configure MPU memory domain
 *
 * This function configures per thread memory domain reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_mem_domain(struct k_thread *thread);

void configure_mpu_user_context(struct k_thread *thread);
#endif

void configure_mpu_thread(struct k_thread *thread);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_MPU_ARC_CORE_MPU_H_ */
