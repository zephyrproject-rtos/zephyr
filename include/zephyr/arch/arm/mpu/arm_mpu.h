/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_MPU_ARM_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_MPU_ARM_MPU_H_

#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M7) || \
	defined(CONFIG_ARMV7_R)
#include <zephyr/arch/arm/mpu/arm_mpu_v7m.h>
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33) || \
	defined(CONFIG_CPU_CORTEX_M55) || \
	defined(CONFIG_CPU_CORTEX_M85) || \
	defined(CONFIG_AARCH32_ARMV8_R)
#include <zephyr/arch/arm/mpu/arm_mpu_v8.h>
#else
#error "Unsupported ARM CPU"
#endif

#ifndef _ASMLANGUAGE

/* Region definition data structure */
struct arm_mpu_region {
	/* Region Base Address */
	uint32_t base;
	/* Region Name */
	const char *name;
#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	/* Region Size */
	uint32_t size;
#endif
	/* Region Attributes */
	arm_mpu_region_attr_t attr;
};

/* MPU configuration data structure */
struct arm_mpu_config {
	/* Number of regions */
	uint32_t num_regions;
	/* Regions */
	const struct arm_mpu_region *mpu_regions;
};

#if defined(CONFIG_ARMV7_R)
#define MPU_REGION_ENTRY(_name, _base, _size, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.size = _size, \
		.attr = _attr, \
	}
#else
#define MPU_REGION_ENTRY(_name, _base, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.attr = _attr, \
	}
#endif

/* Reference to the MPU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MPU
 * regions enabled during kernel initialization. Dynamic MPU regions (e.g.
 * for Thread Stack, Stack Guards, etc.) are programmed during runtime, thus,
 * not kept here.
 */
extern const struct arm_mpu_config mpu_config;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_MPU_ARM_MPU_H_ */
