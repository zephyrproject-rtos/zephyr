/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _NXP_MPU_H_
#define _NXP_MPU_H_

#include <fsl_common.h>
#include <arch/arm/cortex_m/mpu/arm_core_mpu_dev.h>

#define NXP_MPU_BASE SYSMPU_BASE

#define NXP_MPU_REGION_NUMBER 12

/* Read Attribute */
#define MPU_REGION_READ  ((1 << 2) | (1 << 8) | (1 << 14))

/* Write Attribute */
#define MPU_REGION_WRITE ((1 << 1) | (1 << 7) | (1 << 13))

/* Execute Attribute */
#define MPU_REGION_EXEC  ((1 << 0) | (1 << 6) | (1 << 12))

/* Super User Attributes */
#define MPU_REGION_SU    ((3 << 3) | (3 << 9) | (3 << 15) | (3 << 21))

/* Some helper defines for common regions */
#define REGION_RAM_ATTR	(MPU_REGION_READ | MPU_REGION_WRITE | MPU_REGION_SU)
#define REGION_FLASH_ATTR (MPU_REGION_READ | MPU_REGION_EXEC | MPU_REGION_SU)
#define REGION_IO_ATTR (MPU_REGION_READ | MPU_REGION_WRITE | \
			MPU_REGION_EXEC |  MPU_REGION_SU)

/* Region definition data structure */
struct nxp_mpu_region {
	/* Region Base Address */
	u32_t base;
	/* Region End Address */
	u32_t end;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	u32_t attr;
};

#define MPU_REGION_ENTRY(_name, _base, _end, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.end = _end, \
		.attr = _attr, \
	}

/* MPU configuration data structure */
struct nxp_mpu_config {
	/* Number of regions */
	u32_t num_regions;
	/* Regions */
	struct nxp_mpu_region *mpu_regions;
};

/* Reference to the MPU configuration */
extern struct nxp_mpu_config mpu_config;

#endif /* _NXP_MPU_H_ */
