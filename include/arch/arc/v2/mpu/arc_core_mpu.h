/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ARC_CORE_MPU_H_
#define _ARC_CORE_MPU_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The defines below represent the region types. The MPU driver is responsible
 * to allocate the region accordingly to the type and set the correct
 * attributes.
 *
 * Each MPU is different and has a different set of attributes, hence instead
 * of having the attributes at this level the arm_mpu_core defines the intent
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
/* Thread Stack Region Intent Type */
#define THREAD_STACK_REGION 0x1
#define THREAD_STACK_GUARD_REGION 0x2

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
 * Before configure the MPU regions, mpu should be disabled
 */
/**
 * @brief configure the default region
 *
 * @param   region_attr region attribute of default region
 */
void arc_core_mpu_default(u32_t region_attr);

/**
 * @brief configure the mpu region
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

#ifdef __cplusplus
}
#endif

#endif /* _ARC_CORE_MPU_H_ */
