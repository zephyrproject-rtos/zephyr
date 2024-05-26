/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#define USBHS_BASE	DT_REG_ADDR_BY_NAME(DT_NODELABEL(usbhs), core)
#define USBHS_SIZE	DT_REG_SIZE_BY_NAME(DT_NODELABEL(usbhs), core)

static struct arm_mpu_region mpu_regions[] = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usbhs), okay)
	MPU_REGION_ENTRY("USBHS_CORE", USBHS_BASE,
			 REGION_RAM_NOCACHE_ATTR(USBHS_BASE, USBHS_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
