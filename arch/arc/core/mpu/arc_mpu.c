/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arc/v2/aux_regs.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#include <linker/linker-defs.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpu);

/**
 * @brief Get the number of supported MPU regions
 *
 */
static inline uint8_t get_num_regions(void)
{
	uint32_t num = z_arc_v2_aux_reg_read(_ARC_V2_MPU_BUILD);

	num = (num & 0xFF00U) >> 8U;

	return (uint8_t)num;
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct parameter set.
 */
static inline uint32_t get_region_attr_by_type(uint32_t type)
{
	switch (type) {
	case THREAD_STACK_USER_REGION:
		return REGION_RAM_ATTR;
	case THREAD_STACK_REGION:
		return AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR;
	case THREAD_APP_DATA_REGION:
		return REGION_RAM_ATTR;
	case THREAD_STACK_GUARD_REGION:
		/* no Write and Execute to guard region */
		return AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR;
	default:
		/* unknown type */
		return 0;
	}
}

#if CONFIG_ARC_MPU_VER == 2
#include "arc_mpu_v2_internal.h"
#elif CONFIG_ARC_MPU_VER == 4
#include "arc_mpu_v4_internal.h"
#endif
