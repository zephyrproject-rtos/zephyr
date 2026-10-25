/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/tricore/mpu.h>

static const struct tricore_mpu_region sections[] = {
	{
		.name = "FLASH",
		.start = CONFIG_FLASH_BASE_ADDRESS,
		.end = CONFIG_FLASH_BASE_ADDRESS + (CONFIG_FLASH_SIZE * 1024),
		.flags = TRICORE_MPU_ACCESS_P_RX_U_RX,
	},
	{
		.name = "SRAM",
		.start = CONFIG_SRAM_BASE_ADDRESS,
		.end = CONFIG_SRAM_BASE_ADDRESS + (CONFIG_SRAM_SIZE * 1024),
		.flags = TRICORE_MPU_ACCESS_P_RW_U_NA,
	},
	{
		.name = "PERIPHERALS",
		.start = 0xF0000000U,
		.end = 0xFFFFFFFFU,
		.flags = TRICORE_MPU_ACCESS_P_RW_U_NA,
	},
};

const struct tricore_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(sections),
	.regions = sections,
};
