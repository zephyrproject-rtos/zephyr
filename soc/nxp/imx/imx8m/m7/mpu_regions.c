/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#define REGION_MASK_BASE_ADDRESS			0x00000000U
#define REGION_ITCM_BASE_ADDRESS			0x00000000U
#define REGION_QSPI_BASE_ADDRESS			0x08000000U
#define REGION_DTCM_BASE_ADDRESS			0x20000000U
#define REGION_DDR_BASE_ADDRESS				0x40000000U
#define REGION_DDR2_BASE_ADDRESS			0x80000000U
#if defined(CONFIG_CODE_DDR)
#define REGION_DDR_NONCACHE_BASE_ADDRESS	0x80000000U
#define REGION_DDR_NONCACHE_SIZE			0x00400000U
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/*
	 * Region 0 [0x0000_0000 - 0x4000_0000]:
	 * Memory with Device type, not executable, not shareable, non-cacheable.
	 */
	MPU_REGION_ENTRY("MASK", REGION_MASK_BASE_ADDRESS,
					 { ARM_MPU_RASR(1, ARM_MPU_AP_FULL,
					 0, 0, 0, 1, 0, ARM_MPU_REGION_SIZE_1GB) }),

	/*
	 * Region 1 ITCM[0x0000_0000 - 0x0001_FFFF]:
	 * Memory with Normal type, not shareable, non-cacheable
	 */
	MPU_REGION_ENTRY("ITCM", REGION_ITCM_BASE_ADDRESS,
					 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
					 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_128KB) }),

	/*
	 * Region 2 QSPI[0x0800_0000 - 0x0FFF_FFFF]:
	 * Memory with Normal type, not shareable, cacheable
	 */
	MPU_REGION_ENTRY("QSPI", REGION_QSPI_BASE_ADDRESS,
					 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
					 1, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_128MB) }),

	/*
	 * Region 3 DTCM[0x2000_0000 - 0x2002_0000]:
	 * Memory with Normal type, not shareable, non-cacheable
	 */
	MPU_REGION_ENTRY("DTCM", REGION_DTCM_BASE_ADDRESS,
					 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
					 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_128KB) }),

	/*
	 * Region 4 DDR[0x4000_0000 - 0x8000_0000]:
	 * Memory with Normal type, not shareable, non-cacheable
	 */
	MPU_REGION_ENTRY("DDR", REGION_DDR_BASE_ADDRESS,
					 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
					 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB) }),

	/*
	 * Non-cacheable area is provided in DDR memory, the DDR region [0x80000000 ~ 0x81000000]
	 * (please see the imx8mp-evk-rpmsg.dts) totally 16MB is reserved for CM7 core. You can put
	 * global or static uninitialized variables in NonCacheable section(initialized variables in
	 * NonCacheable.init section) to make them uncacheable. Since the base address of MPU region
	 * should be multiples of region size, to make it simple, the MPU region 5 set the address
	 * space 0x80000000 ~ 0xBFFFFFFF to be non-cacheable. Then MPU region 6 set the text and
	 * data section to be cacheable if the program running on DDR. The cacheable area base
	 * address should be multiples of its size in linker file, they can be modified per your
	 * needs.
	 *
	 * Region 5 DDR[0x8000_0000 - 0xBFFFFFFF]:
	 * Memory with Normal type, not shareable, non-cacheable
	 */
	MPU_REGION_ENTRY("DDR2", REGION_DDR2_BASE_ADDRESS,
					 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
					 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB) }),

#if defined(CONFIG_CODE_DDR)
	/* If run on DDR, configure text and data section to be cacheable */
	MPU_REGION_ENTRY("DDR_NONCACHE", REGION_DDR_NONCACHE_BASE_ADDRESS,
					 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1,
					 0, 1, 1, 0, REGION_DDR_NONCACHE_SIZE) }),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
