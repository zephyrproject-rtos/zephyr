/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset interface for Actions SoC
 */

#include <kernel.h>
#include "soc_regs.h"
#include "soc_reset.h"

/*
 * Reset functions should be placed in RAM rather than in Flash. This is
 * required for resetting the SPI interface, otherwise we will get Xip error
 * as the the code is running out of NOR Flash.
 */
__ramfunc static void acts_reset_peripheral_control(int reset_id, int assert)
{
	if (reset_id > RESET_ID_MAX_ID)
		return;

	if (assert)
		sys_write32(sys_read32(CMU_DEVRST_REG) & ~(1 << reset_id),
			    CMU_DEVRST_REG);
	else
		sys_write32(sys_read32(CMU_DEVRST_REG) | (1 << reset_id),
			    CMU_DEVRST_REG);
}

__ramfunc void acts_reset_peripheral_assert(int reset_id)
{
	acts_reset_peripheral_control(reset_id, 1);
}

__ramfunc void acts_reset_peripheral_deassert(int reset_id)
{
	acts_reset_peripheral_control(reset_id, 0);
}

__ramfunc void acts_reset_peripheral(int reset_id)
{
	acts_reset_peripheral_assert(reset_id);
	acts_reset_peripheral_deassert(reset_id);
}
