/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/cortex_a_r/mpu.h>

/*
 * Code memory attribute: cacheable, bufferable, non-shareable, executable
 */
#define REGION_CACHE_CODE_DATA_ATTR {(NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE | P_RW_U_RW_Msk)}

/*
 * Peripheral attribute: non-cacheable, shareable, no execute
 */
#define REGION_PERIPH_ATTR {(STRONGLY_ORDERED_SHAREABLE | MPU_RASR_XN_Msk | P_RW_U_RW_Msk)}

/*
 * Shared memory attribute: non-cacheable, TEX=1, shareable, no execute
 */
#define REGION_NON_CACHED_DATA_ATTR                                                                \
	{(NORMAL_OUTER_INNER_NON_CACHEABLE_SHAREABLE | MPU_RASR_XN_Msk | P_RW_U_RW_Msk)}

const struct arm_mpu_region mpu_regions[] = {
	/* Region 0: Base system region - 2GB */
	MPU_REGION_ENTRY("SYSTEM",          /* name */
			 0x0,               /* base address */
			 REGION_2G,         /* size */
			 REGION_PERIPH_ATTR /* attributes */
			 ),
#ifdef CONFIG_CORES_IN_LOCKSTEP_MODE
	/* Region 1: ATCM - 256 KB */
	MPU_REGION_ENTRY("ATCM",                     /* name */
			 0x0,                        /* base address */
			 REGION_256K,                /* size */
			 REGION_CACHE_CODE_DATA_ATTR /* attributes */
			 ),
	/* Region 2: BTCM - 256 KB */
	MPU_REGION_ENTRY("BTCM",                     /* name */
			 0x80000,                    /* base address */
			 REGION_256K,                /* size */
			 REGION_CACHE_CODE_DATA_ATTR /* attributes */
			 ),
#else  /* Dual Core Mode - Default */
	/* Region 1: ATCM - 128 KB */
	MPU_REGION_ENTRY("ATCM",                     /* name */
			 0,                          /* base address */
			 REGION_128K,                /* size */
			 REGION_CACHE_CODE_DATA_ATTR /* attributes */
			 ),
	/* Region 2: BTCM - 128 KB */
	MPU_REGION_ENTRY("BTCM",                     /* name */
			 0x80000,                    /* base address */
			 REGION_128K,                /* size */
			 REGION_CACHE_CODE_DATA_ATTR /* attributes */
			 ),
#endif /* CONFIG_CORES_IN_LOCKSTEP_MODE */

#ifdef CONFIG_SOC_AM261X_R5F0_0
	/* Region 3: OCRAM region - 2MB */
	MPU_REGION_ENTRY("RAM",                      /* name */
			 0x70000000,                 /* base address */
			 REGION_2M,                  /* size */
			 REGION_CACHE_CODE_DATA_ATTR /* attributes */
			 ),
#else /* defaults to CONFIG_SOC_AM261X_R5F0_1 */
	/* Region 3: OCRAM region - 2MB */
	MPU_REGION_ENTRY("RAM",                      /* name */
			 0x70000000,                 /* base address */
			 REGION_2M,                  /* size */
			 REGION_CACHE_CODE_DATA_ATTR /* attributes */
			 ),
#endif
	/* Region 4: Peripheral region - 16KB */
	MPU_REGION_ENTRY("PERIPHERAL",      /* name */
			 0x50D00000,        /* base address */
			 REGION_16K,        /* size */
			 REGION_PERIPH_ATTR /* attributes */
			 ),
	/* Region 5: Shared memory region - 16KB */
	MPU_REGION_ENTRY("SHAREDMEM",                /* name */
			 0x72000000,                 /* base address */
			 REGION_16K,                 /* size */
			 REGION_NON_CACHED_DATA_ATTR /* attributes */
			 )};

/* MPU Configuration */
const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
