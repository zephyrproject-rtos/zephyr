/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

#define ARM_MPU_DEV ((volatile struct arm_mpu *) ARM_MPU_BASE)

static inline u8_t _get_num_regions(void)
{
	u32_t type = ARM_MPU_DEV->type;

	type = (type & 0xFF00) >> 8;

	return (u8_t)type;
}

static void _region_init(u32_t index, u32_t region_addr,
			 u32_t region_attr)
{
	/* Select the region you want to access */
	ARM_MPU_DEV->rnr = index;
	/* Configure the region */
	ARM_MPU_DEV->rbar = region_addr | REGION_VALID | index;
	ARM_MPU_DEV->rasr = region_attr | REGION_ENABLE;
}

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static void _arm_mpu_config(void)
{
	u32_t r_index;

	/* ARM MPU supports up to 16 Regions */
	if (mpu_config.num_regions > _get_num_regions()) {
		return;
	}

	/* Disable MPU */
	ARM_MPU_DEV->ctrl = 0;

	/* Configure regions */
	for (r_index = 0; r_index < mpu_config.num_regions; r_index++) {
		_region_init(r_index,
			     mpu_config.mpu_regions[r_index].base,
			     mpu_config.mpu_regions[r_index].attr);
	}

	/* Enable MPU */
	ARM_MPU_DEV->ctrl = 1;

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

static int arm_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_arm_mpu_config();

	return 0;
}

SYS_INIT(arm_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
