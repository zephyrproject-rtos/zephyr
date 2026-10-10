/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#define DEVICE_REGION0_START (0x00000000u)
#define DEVICE_REGION0_LIMIT (0x3fffffffu)
#define DEVICE_REGION1_START (0xc0000000u)
#define DEVICE_REGION1_LIMIT (0xffffffffu)

extern const uint32_t __rodata_region_start;
extern const uint32_t _image_ram_start;

static const struct arm_mpu_region mpu_regions[] = {
	/* Peripherals */
	MPU_REGION_ENTRY("DEVICE0", DEVICE_REGION0_START,
			 REGION_DEVICE_ATTR(DEVICE_REGION0_LIMIT)),
	MPU_REGION_ENTRY("DEVICE1", DEVICE_REGION1_START,
			 REGION_DEVICE_ATTR(DEVICE_REGION1_LIMIT)),
	/* SDRAM */
	MPU_REGION_ENTRY("TEXT", DT_CHOSEN_SRAM_ADDR,
			 REGION_RAM_TEXT_ATTR((uint32_t)(&__rodata_region_start))),
	MPU_REGION_ENTRY("RODATA", (uint32_t)(&__rodata_region_start),
			 REGION_RAM_RO_ATTR((uint32_t)(&_image_ram_start))),
	MPU_REGION_ENTRY("SRAM", (uint32_t)(&_image_ram_start),
			 REGION_RAM_ATTR(DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
