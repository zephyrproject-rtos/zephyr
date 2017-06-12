/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ARM_MPU_H_
#define _ARM_MPU_H_

#include <arch/arm/cortex_m/mpu/arm_core_mpu_dev.h>

struct arm_mpu {
	/* 0xE000ED90 MPU Type Register */
	volatile u32_t type;
	/* 0xE000ED94 MPU Control Register */
	volatile u32_t ctrl;
	/* 0xE000ED98 MPU Region Number Register */
	volatile u32_t rnr;
	/* 0xE000ED9C MPU Region Base Address Register */
	volatile u32_t rbar;
	/* 0xE000EDA0 MPU Region Attribute and Size Register */
	volatile u32_t rasr;
	/* 0xE000EDA4 Alias of RBAR */
	volatile u32_t rbar_a1;
	/* 0xE000EDA8 Alias of RASR */
	volatile u32_t rasr_a1;
	/* 0xE000EDAC Alias of RBAR */
	volatile u32_t rbar_a2;
	/* 0xE000EDB0 Alias of RASR */
	volatile u32_t rasr_a2;
	/* 0xE000EDB4 Alias of RBAR */
	volatile u32_t rbar_a3;
	/* 0xE000EDB8 Alias of RASR */
	volatile u32_t rasr_a3;
};

#define ARM_MPU_BASE	0xE000ED90

/* ARM MPU CTRL Register */
/* Enable MPU */
#define ARM_MPU_ENABLE		(1 << 0)
/* Enable MPU during hard fault, NMI, and FAULTMASK handlers */
#define ARM_MPU_HFNMIENA	(1 << 1)
/* Enable privileged software access to the default memory map */
#define ARM_MPU_PRIVDEFENA	(1 << 2)

#define REGION_VALID	(1 << 4)
/* ARM MPU RBAR Register */
/* Region base address mask */
#define REGION_BASE_ADDR_MASK	0xFFFFFFE0

/* eXecute Never */
#define NOT_EXEC        (0x1 << 28)

/* Privileged No Access, Unprivileged No Access */
#define NO_ACCESS	(0x0 << 24)
/* Privileged No Access, Unprivileged No Access */
#define P_NA_U_NA	(0x0 << 24)
/* Privileged Read Write, Unprivileged No Access */
#define P_RW_U_NA	(0x1 << 24)
/* Privileged Read Write, Unprivileged Read Only */
#define P_RW_U_RO	(0x2 << 24)
/* Privileged Read Write, Unprivileged Read Write */
#define P_RW_U_RW	(0x3 << 24)
/* Privileged Read Write, Unprivileged Read Write */
#define FULL_ACCESS	(0x3 << 24)
/* Privileged Read Only, Unprivileged No Access */
#define P_RO_U_NA	(0x5 << 24)
/* Privileged Read Only, Unprivileged Read Only */
#define P_RO_U_RO	(0x6 << 24)
/* Privileged Read Only, Unprivileged Read Only */
#define RO		(0x7 << 24)

#define STRONGLY_ORDERED_SHAREABLE			(1 << 18)
#define DEVICE_SHAREABLE				((1 << 16) | (1 << 18))
#define NORMAL_OUTER_INNER_WRITE_THROUGH_SHAREABLE	((1 << 17) | (1 << 18))
#define NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE	(1 << 17)
#define NORMAL_OUTER_INNER_WRITE_BACK_SHAREABLE	\
				((1 << 17) | (1 << 16) | (1 << 18))
#define NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE	((1 << 17) | (1 << 16))
#define NORMAL_OUTER_INNER_NON_CACHEABLE_SHAREABLE	((1 << 19) | (1 << 18))
#define NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE	(1 << 19)
#define NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_SHAREABLE \
				((1 << 19) | (1 << 17) | (1 << 16) | (1 << 18))
#define NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NONSHAREABLE \
				((1 << 19) | (1 << 17) | (1 << 18))
#define DEVICE_NON_SHAREABLE				(2 << 19)

/* Some helper defines for common regions */
#define REGION_RAM_ATTR(size) \
		(NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE | \
		 NOT_EXEC | size | FULL_ACCESS)
#define REGION_FLASH_ATTR(size) \
		(NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE | size | RO)
#define REGION_PPB_ATTR(size) (STRONGLY_ORDERED_SHAREABLE | size | FULL_ACCESS)
#define REGION_IO_ATTR(size) (DEVICE_NON_SHAREABLE | size | FULL_ACCESS)

#define SUB_REGION_0_DISABLED	(0x01 << 8)
#define SUB_REGION_1_DISABLED	(0x02 << 8)
#define SUB_REGION_2_DISABLED	(0x04 << 8)
#define SUB_REGION_3_DISABLED	(0x08 << 8)
#define SUB_REGION_4_DISABLED	(0x10 << 8)
#define SUB_REGION_5_DISABLED	(0x20 << 8)
#define SUB_REGION_6_DISABLED	(0x40 << 8)
#define SUB_REGION_7_DISABLED	(0x80 << 8)

#define REGION_32B      (0x04 << 1)
#define REGION_64B      (0x05 << 1)
#define REGION_128B     (0x06 << 1)
#define REGION_256B     (0x07 << 1)
#define REGION_512B     (0x08 << 1)
#define REGION_1K       (0x09 << 1)
#define REGION_2K       (0x0A << 1)
#define REGION_4K       (0x0B << 1)
#define REGION_8K       (0x0C << 1)
#define REGION_16K      (0x0D << 1)
#define REGION_32K      (0x0E << 1)
#define REGION_64K      (0x0F << 1)
#define REGION_128K     (0x10 << 1)
#define REGION_256K     (0x11 << 1)
#define REGION_512K     (0x12 << 1)
#define REGION_1M       (0x13 << 1)
#define REGION_2M       (0x14 << 1)
#define REGION_4M       (0x15 << 1)
#define REGION_8M       (0x16 << 1)
#define REGION_16M      (0x17 << 1)
#define REGION_32M      (0x18 << 1)
#define REGION_64M      (0x19 << 1)
#define REGION_128M     (0x1A << 1)
#define REGION_256M     (0x1B << 1)
#define REGION_512M     (0x1C << 1)
#define REGION_1G       (0x1D << 1)
#define REGION_2G       (0x1E << 1)
#define REGION_4G       (0x1F << 1)

#define REGION_ENABLE	(1 << 0)

/* Region definition data structure */
struct arm_mpu_region {
	/* Region Base Address */
	u32_t base;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	u32_t attr;
};

#define MPU_REGION_ENTRY(_name, _base, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.attr = _attr, \
	}

/* MPU configuration data structure */
struct arm_mpu_config {
	/* Number of regions */
	u32_t num_regions;
	/* Regions */
	struct arm_mpu_region *mpu_regions;
};

/* Reference to the MPU configuration */
extern struct arm_mpu_config mpu_config;

#endif /* _ARM_MPU_H_ */
