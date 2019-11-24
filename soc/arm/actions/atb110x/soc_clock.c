/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Peripheral reset interface for ATB110X
 */

#include <kernel.h>
#include "soc_clock.h"
#include "soc_pmu.h"

void acts_request_rc_3M(bool enable)
{
	int val;

	if (enable) {
		acts_request_vd12(true);

		/* Enable 3MHz clock */
		sys_write32(sys_read32(ACT_3M_CTL) | (1 << ACT_3M_CTL_RC3MEN),
			    ACT_3M_CTL);

		/* Block until the clock is stable */
		do {
			val = sys_read32(ACT_3M_CTL);
		} while (!(val & (1 << ACT_3M_CTL_RC3M_OK)));
	} else {
		/* Disable 3MHz clock */
		sys_write32(sys_read32(ACT_3M_CTL) & ~(1 << ACT_3M_CTL_RC3MEN),
			    ACT_3M_CTL);
		acts_request_vd12(false);
	}
}
