/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

/* clang-format off */

#define MPUTYPE_READ_ONLY \
	{ \
		.rasr = (P_RO_U_RO_Msk \
				| (7 << MPU_RASR_TEX_Pos) \
				| MPU_RASR_C_Msk \
				| MPU_RASR_B_Msk \
				| MPU_RASR_XN_Msk) \
	}

#define MPUTYPE_READ_ONLY_PRIV \
	{ \
		.rasr = (P_RO_U_RO_Msk \
				| (5 << MPU_RASR_TEX_Pos) \
				| MPU_RASR_B_Msk) \
	}

#define MPUTYPE_PRIV_WBWACACHE_XN \
	{ \
		.rasr = (P_RW_U_NA_Msk \
				| (5 << MPU_RASR_TEX_Pos) \
				| MPU_RASR_B_Msk \
				| MPU_RASR_XN_Msk) \
	}

#define MPUTYPE_PRIV_DEVICE \
	{ \
		.rasr = (P_RW_U_NA_Msk \
				| (2 << MPU_RASR_TEX_Pos) \
				| MPU_RASR_XN_Msk \
				| MPU_RASR_B_Msk \
				| MPU_RASR_S_Msk) \
	}

/* clang-format on */

extern uint32_t _image_rom_end_order;
static const struct arm_mpu_region mpu_regions[] = {

	/* clang-format off */

	MPU_REGION_ENTRY("SRAM",
			0x00000000,
			REGION_256M,
		MPUTYPE_PRIV_WBWACACHE_XN),

	MPU_REGION_ENTRY("SRAM",
			0x00000000,
			((uint32_t)&_image_rom_end_order),
			MPUTYPE_READ_ONLY_PRIV),

	MPU_REGION_ENTRY("REGISTERS",
			0x10000000,
			REGION_256M,
			MPUTYPE_PRIV_DEVICE),

	MPU_REGION_ENTRY("FLASH",
			0x20000000,
			REGION_256M,
			MPUTYPE_READ_ONLY),

	/* clang-format on */
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
