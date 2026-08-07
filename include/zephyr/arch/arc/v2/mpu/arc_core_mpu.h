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

/**
 * @brief configure the thread's mpu regions
 *
 * @param thread the target thread
 */
void arc_core_mpu_configure_thread(struct k_thread *thread);

/*
 * Before configure the MPU regions, MPU should be disabled
 */
/**
 * @brief configure the default region
 *
 * @param   region_attr region attribute of default region
 */
void arc_core_mpu_default(uint32_t region_attr);

/**
 * @brief configure the MPU region
 *
 * @param   index   MPU region index
 * @param   base    base address
 * @param   size    size of region
 * @param   region_attr region attribute
 */
int arc_core_mpu_region(uint32_t index, uint32_t base, uint32_t size,
			 uint32_t region_attr);

#endif /* CONFIG_ARC_CORE_MPU */

#if defined(CONFIG_USERSPACE)
void arc_core_mpu_configure_mem_domain(struct k_thread *thread);
void arc_core_mpu_remove_mem_domain(struct k_mem_domain *mem_domain);
void arc_core_mpu_remove_mem_partition(struct k_mem_domain *domain,
			uint32_t partition_id);
int arc_core_mpu_get_max_domain_partition_regions(void);
int arc_core_mpu_buffer_validate(const void *addr, size_t size, int write);

#endif

void configure_mpu_thread(struct k_thread *thread);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_MPU_ARC_CORE_MPU_H_ */
