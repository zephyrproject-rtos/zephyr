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

/* Bus Master User Mode Access */
#define UM_READ		4
#define UM_WRITE	2
#define UM_EXEC		1

#define BM0_UM_SHIFT	0
#define BM1_UM_SHIFT	6
#define BM2_UM_SHIFT	12
#define BM3_UM_SHIFT	18

/* Bus Master Supervisor Mode Access */
#define SM_RWX_ALLOW	0
#define SM_RX_ALLOW	1
#define SM_RW_ALLOW	2
#define SM_SAME_AS_UM	3

#define BM0_SM_SHIFT	3
#define BM1_SM_SHIFT	9
#define BM2_SM_SHIFT	15
#define BM3_SM_SHIFT	21

/* Read Attribute */
#define MPU_REGION_READ  ((UM_READ << BM0_UM_SHIFT) | \
			  (UM_READ << BM1_UM_SHIFT) | \
			  (UM_READ << BM2_UM_SHIFT) | \
			  (UM_READ << BM3_UM_SHIFT))

/* Write Attribute */
#define MPU_REGION_WRITE ((UM_WRITE << BM0_UM_SHIFT) | \
			  (UM_WRITE << BM1_UM_SHIFT) | \
			  (UM_WRITE << BM2_UM_SHIFT) | \
			  (UM_WRITE << BM3_UM_SHIFT))

/* Execute Attribute */
#define MPU_REGION_EXEC  ((UM_EXEC << BM0_UM_SHIFT) | \
			  (UM_EXEC << BM1_UM_SHIFT) | \
			  (UM_EXEC << BM2_UM_SHIFT) | \
			  (UM_EXEC << BM3_UM_SHIFT))

/* Super User Attributes */
#define MPU_REGION_SU	 ((SM_SAME_AS_UM << BM0_SM_SHIFT) | \
			  (SM_SAME_AS_UM << BM1_SM_SHIFT) | \
			  (SM_SAME_AS_UM << BM2_SM_SHIFT) | \
			  (SM_SAME_AS_UM << BM3_SM_SHIFT))

#define MPU_REGION_SU_RX ((SM_RX_ALLOW << BM0_SM_SHIFT) | \
			  (SM_RX_ALLOW << BM1_SM_SHIFT) | \
			  (SM_RX_ALLOW << BM2_SM_SHIFT) | \
			  (SM_RX_ALLOW << BM3_SM_SHIFT))

#define MPU_REGION_SU_RW ((SM_RW_ALLOW << BM0_SM_SHIFT) | \
			  (SM_RW_ALLOW << BM1_SM_SHIFT) | \
			  (SM_RW_ALLOW << BM2_SM_SHIFT) | \
			  (SM_RW_ALLOW << BM3_SM_SHIFT))

#define MPU_REGION_SU_RWX ((SM_RWX_ALLOW << BM0_SM_SHIFT) | \
			   (SM_RWX_ALLOW << BM1_SM_SHIFT) | \
			   (SM_RWX_ALLOW << BM2_SM_SHIFT) | \
			   (SM_RWX_ALLOW << BM3_SM_SHIFT))

/* The ENDADDR field has the last 5 bit reserved and set to 1 */
#define ENDADDR_ROUND(x) (x - 0x1F)

/* Some helper defines for common regions */
#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
#define REGION_RAM_ATTR	  (MPU_REGION_READ | \
			   MPU_REGION_WRITE | \
			   MPU_REGION_EXEC | \
			   MPU_REGION_SU)

#define REGION_FLASH_ATTR (MPU_REGION_READ | \
			   MPU_REGION_WRITE | \
			   MPU_REGION_EXEC | \
			   MPU_REGION_SU)
#else
#define REGION_RAM_ATTR	  (MPU_REGION_READ | \
			   MPU_REGION_WRITE | \
			   MPU_REGION_SU)

#define REGION_FLASH_ATTR (MPU_REGION_READ | \
			   MPU_REGION_EXEC | \
			   MPU_REGION_SU)
#endif

#define REGION_IO_ATTR	  (MPU_REGION_READ | \
			   MPU_REGION_WRITE | \
			   MPU_REGION_EXEC | \
			   MPU_REGION_SU)

#define REGION_RO_ATTR	  (MPU_REGION_READ | \
			   MPU_REGION_SU)

#define REGION_DEBUG_ATTR  MPU_REGION_SU

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
	/* SRAM Region */
	u32_t sram_region;
};

/* Reference to the MPU configuration */
extern struct nxp_mpu_config mpu_config;

#endif /* _NXP_MPU_H_ */
