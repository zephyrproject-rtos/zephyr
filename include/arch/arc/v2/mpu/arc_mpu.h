/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_MPU_ARC_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_MPU_ARC_MPU_H_



#define AUX_MPU_RDP_UE  0x008    /* allow user execution */
#define AUX_MPU_RDP_UW  0x010    /* allow user write */
#define AUX_MPU_RDP_UR  0x020    /* allow user read */
#define AUX_MPU_RDP_KE  0x040    /* only allow kernel execution */
#define AUX_MPU_RDP_KW  0x080    /* only allow kernel write */
#define AUX_MPU_RDP_KR  0x100    /* only allow kernel read */
#define AUX_MPU_RDP_S   0x8000   /* secure */
#define AUX_MPU_RDP_N   0x0000   /* normal */



/* Some helper defines for common regions */
#define REGION_RAM_ATTR	\
			(AUX_MPU_RDP_UW | AUX_MPU_RDP_UR | \
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR)

#define REGION_FLASH_ATTR \
			(AUX_MPU_RDP_UE | AUX_MPU_RDP_UR | \
			 AUX_MPU_RDP_KE | AUX_MPU_RDP_KR)

#define REGION_IO_ATTR \
			(AUX_MPU_RDP_UW | AUX_MPU_RDP_UR | \
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR)

#define REGION_ALL_ATTR \
			(AUX_MPU_RDP_UW | AUX_MPU_RDP_UR | \
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR | \
			 AUX_MPU_RDP_KE | AUX_MPU_RDP_UE)


#define REGION_32B      0x200
#define REGION_64B      0x201
#define REGION_128B     0x202
#define REGION_256B     0x203
#define REGION_512B     0x400
#define REGION_1K       0x401
#define REGION_2K       0x402
#define REGION_4K       0x403
#define REGION_8K       0x600
#define REGION_16K      0x601
#define REGION_32K      0x602
#define REGION_64K      0x603
#define REGION_128K     0x800
#define REGION_256K     0x801
#define REGION_512K     0x802
#define REGION_1M       0x803
#define REGION_2M       0xA00
#define REGION_4M       0xA01
#define REGION_8M       0xA02
#define REGION_16M      0xA03
#define REGION_32M      0xC00
#define REGION_64M      0xC01
#define REGION_128M     0xC02
#define REGION_256M     0xC03
#define REGION_512M     0xE00
#define REGION_1G       0xE01
#define REGION_2G       0xE02
#define REGION_4G       0xE03

/* Region definition data structure */
struct arc_mpu_region {
	/* Region Name */
	const char *name;
	/* Region Base Address */
	u32_t base;
	u32_t size;
	/* Region Attributes */
	u32_t attr;
};

#define MPU_REGION_ENTRY(_name, _base, _size, _attr) \
	{\
		.name = _name, \
		.base = _base, \
		.size = _size, \
		.attr = _attr, \
	}

/* MPU configuration data structure */
struct arc_mpu_config {
	/* Number of regions */
	u32_t num_regions;
	/* Regions */
	struct arc_mpu_region *mpu_regions;
};

/* Reference to the MPU configuration */
extern struct arc_mpu_config mpu_config;

#endif /* _ARC_CORE_MPU_H_ */
