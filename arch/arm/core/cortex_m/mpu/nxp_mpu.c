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
#include <arch/arm/cortex_m/mpu/nxp_mpu.h>
#include <logging/sys_log.h>
#include <misc/__assert.h>

static inline u8_t _get_num_regions(void)
{
	u32_t type = (SYSMPU->CESR & SYSMPU_CESR_NRGD_MASK)
			>> SYSMPU_CESR_NRGD_SHIFT;

	switch (type) {
	case 0:
		return 8;
	case 1:
		return 12;
	case 2:
		return 16;
	default:
		__ASSERT(0, "Unsupported MPU configuration.");
		return 0;
	}

	return NXP_MPU_REGION_NUMBER;
}

static void _region_init(u32_t index, u32_t region_base,
			 u32_t region_end, u32_t region_attr)
{
	SYSMPU->WORD[index][0] = region_base;
	SYSMPU->WORD[index][1] = region_end;
	SYSMPU->WORD[index][2] = region_attr;
	SYSMPU->WORD[index][3] = SYSMPU_WORD_VLD_MASK;

	SYS_LOG_DBG("[%d] 0x%08x 0x%08x 0x%08x 0x%08x", index,
		    SYSMPU->WORD[index][0],
		    SYSMPU->WORD[index][1],
		    SYSMPU->WORD[index][2],
		    SYSMPU->WORD[index][3]);
}

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static void _nxp_mpu_config(void)
{
	u32_t r_index;

	SYS_LOG_DBG("region number: %d", _get_num_regions());

	/* NXP MPU supports up to 16 Regions */
	if (mpu_config.num_regions > _get_num_regions()) {
		return;
	}

	/* Disable MPU */
	SYSMPU->CESR &= ~SYSMPU_CESR_VLD_MASK;
	/* Clear Interrupts */
	SYSMPU->CESR |=  SYSMPU_CESR_SPERR_MASK;

	/* MPU Configuration */

	/* Disable Region 0 */
	SYSMPU->WORD[0][2] = 0;

	/*
	 * Configure regions:
	 * r_index starts from 0 but is passed to region_init as r_index + 1,
	 * region 0 is not configurable
	 */
	for (r_index = 0; r_index < mpu_config.num_regions; r_index++) {
		_region_init(r_index + 1,
			     mpu_config.mpu_regions[r_index].base,
			     mpu_config.mpu_regions[r_index].end,
			     mpu_config.mpu_regions[r_index].attr);
	}

	/* Enable MPU */
	SYSMPU->CESR |= SYSMPU_CESR_VLD_MASK;

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

/*
 * @brief MPU clock configuration
 *
 * This function provides the clock configuration for the Memory Protection
 * Unit (MPU).
 */
static void _nxp_mpu_clock_cfg(void)
{
	/* Enable Clock */
	CLOCK_EnableClock(kCLOCK_Sysmpu0);
}

static int nxp_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_nxp_mpu_clock_cfg();

	_nxp_mpu_config();

	return 0;
}

#if defined(CONFIG_SYS_LOG)
/* To have logging the driver needs to be initialized later */
SYS_INIT(nxp_mpu_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else
SYS_INIT(nxp_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
