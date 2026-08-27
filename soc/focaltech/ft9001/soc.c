/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#define XIP_CLKDIV_BOOT 1U

void xip_clock_switch(uint32_t clk_div);

void soc_early_init_hook(void)
{
#ifdef CONFIG_XIP
	xip_clock_switch(XIP_CLKDIV_BOOT);
#endif
}
