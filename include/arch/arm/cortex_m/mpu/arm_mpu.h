/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_MPU_H_

#include <arch/arm/cortex_m/mpu/arm_core_mpu_dev.h>
#include <arch/arm/cortex_m/cmsis.h>

#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M7)
#include <arch/arm/cortex_m/mpu/arm_mpu_v7m.h>
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33)
#include <arch/arm/cortex_m/mpu/arm_mpu_v8m.h>
#else
#error "Unsupported ARM CPU"
#endif

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE
/* Read-Write access permission attributes */
#define K_MEM_PARTITION_P_NA_U_NA	(NO_ACCESS_Msk | NOT_EXEC)
#define K_MEM_PARTITION_P_RW_U_RW	(P_RW_U_RW_Msk | NOT_EXEC)
#define K_MEM_PARTITION_P_RW_U_RO	(P_RW_U_RO_Msk | NOT_EXEC)
#define K_MEM_PARTITION_P_RW_U_NA	(P_RW_U_NA_Msk | NOT_EXEC)
#define K_MEM_PARTITION_P_RO_U_RO	(P_RO_U_RO_Msk | NOT_EXEC)
#define K_MEM_PARTITION_P_RO_U_NA	(P_RO_U_NA_Msk | NOT_EXEC)

/* Execution-allowed attributes */
#define K_MEM_PARTITION_P_RWX_U_RWX	(P_RW_U_RW_Msk)
#define K_MEM_PARTITION_P_RWX_U_RX	(P_RW_U_RO_Msk)
#define K_MEM_PARTITION_P_RX_U_RX	(P_RO_U_RO_Msk)

#define K_MEM_PARTITION_IS_WRITABLE(attr) \
	({ \
		int __is_writable__; \
		switch (attr) { \
		case P_RW_U_RW: \
		case P_RW_U_RO: \
		case P_RW_U_NA: \
			__is_writable__ = 1; \
			break; \
		default: \
			__is_writable__ = 0; \
		} \
		__is_writable__; \
	})
#define K_MEM_PARTITION_IS_EXECUTABLE(attr) \
	(!((attr) & (NOT_EXEC)))
#endif /* _ASMLANGUAGE */
#endif /* USERSPACE */

/* Region definition data structure */
struct arm_mpu_region {
	/* Region Base Address */
	u32_t base;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	arm_mpu_region_attr_t attr;
};

/* MPU configuration data structure */
struct arm_mpu_config {
	/* Number of regions */
	u32_t num_regions;
	/* Regions */
	struct arm_mpu_region *mpu_regions;
};

#define MPU_REGION_ENTRY(_name, _base, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.attr = _attr, \
	}

/* Reference to the MPU configuration */
extern struct arm_mpu_config mpu_config;

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_MPU_H_ */
