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
#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>
#include <logging/sys_log.h>

#define ARM_MPU_DEV ((volatile struct arm_mpu *) ARM_MPU_BASE)

/* ARM MPU Enabled state */
static u8_t arm_mpu_enabled;

/**
 * The attributes referenced in this function are described at:
 * https://goo.gl/hMry3r
 * This function is private to the driver.
 */
static inline u32_t _get_region_attr(u32_t xn, u32_t ap, u32_t tex,
				     u32_t c, u32_t b, u32_t s,
				     u32_t srd, u32_t size)
{
	return ((xn << 28) | (ap) | (tex << 19) | (s << 18)
		| (c << 17) | (b << 16) | (srd << 5) | (size));
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct parameter set.
 */
static inline u32_t _get_region_attr_by_type(u32_t type, u32_t size)
{
	switch (type) {
	case THREAD_STACK_REGION:
		return 0;
	case THREAD_STACK_GUARD_REGION:
		return _get_region_attr(1, P_RO_U_RO, 0, 1, 0,
					1, 0, REGION_32B);
	default:
		/* Size 0 region */
		return 0;
	}
}

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
	ARM_MPU_DEV->rbar = (region_addr & REGION_BASE_ADDR_MASK)
				| REGION_VALID | index;
	ARM_MPU_DEV->rasr = region_attr | REGION_ENABLE;
}

/* ARM Core MPU Driver API Implementation for ARM MPU */

/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void)
{
	if (arm_mpu_enabled == 0) {
		/* Enable MPU */
		ARM_MPU_DEV->ctrl = ARM_MPU_ENABLE | ARM_MPU_PRIVDEFENA;

		arm_mpu_enabled = 1;
	}
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	if (arm_mpu_enabled == 1) {
		/* Disable MPU */
		ARM_MPU_DEV->ctrl = 0;

		arm_mpu_enabled = 0;
	}
}

/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arm_core_mpu_configure(u8_t type, u32_t base, u32_t size)
{
	SYS_LOG_DBG("Region info: 0x%x 0x%x", base, size);
	/*
	 * The new MPU regions are are allocated per type after the statically
	 * configured regions.
	 */
	u32_t region_index = mpu_config.num_regions + type;
	u32_t region_attr = _get_region_attr_by_type(type, size);

	/* ARM MPU supports up to 16 Regions */
	if (region_index > _get_num_regions()) {
		return;
	}

	_region_init(region_index, base, region_attr);
}

/* ARM MPU Driver Initial Setup */

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
	ARM_MPU_DEV->ctrl = ARM_MPU_ENABLE | ARM_MPU_PRIVDEFENA;

	arm_mpu_enabled = 1;

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
