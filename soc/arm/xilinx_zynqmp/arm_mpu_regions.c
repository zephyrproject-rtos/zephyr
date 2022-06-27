/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Lexmark International, Inc.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/aarch32/mpu/arm_mpu.h>

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

#define MPUTYPE_PRIV_WBWACACHE \
	{ \
		.rasr = (P_RW_U_NA_Msk \
				| (5 << MPU_RASR_TEX_Pos) \
				| MPU_RASR_B_Msk) \
	}

#define MPUTYPE_PRIV_DEVICE \
	{ \
		.rasr = (P_RW_U_NA_Msk \
				| (2 << MPU_RASR_TEX_Pos)) \
	}

extern uint32_t _image_rom_end_order;
static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH0",
			0xc0000000,
			REGION_32M,
			MPUTYPE_READ_ONLY),

	MPU_REGION_ENTRY("SRAM_PRIV",
			0x00000000,
			REGION_64M,
			MPUTYPE_PRIV_WBWACACHE),

	MPU_REGION_ENTRY("SRAM",
			0x00000000,
			((uint32_t)&_image_rom_end_order),
			MPUTYPE_READ_ONLY_PRIV),

	MPU_REGION_ENTRY("REGISTERS",
			0xf8000000,
			REGION_128M,
			MPUTYPE_PRIV_DEVICE),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
