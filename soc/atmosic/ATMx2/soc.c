/*
 * Copyright (c) 2021 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>

#include "at_apb_wrpr_regs_core_macro.h"

/**
 * @brief Perform basic hardware initialization at boot.
 */
static int atmosic_atmx2_init(void)
{
	/* Disable init watchdog */
	CMSDK_WATCHDOG->LOCK = 0x1ACCE551;
	CMSDK_WATCHDOG->CTRL = 0;
	CMSDK_WATCHDOG->LOCK = 0;

	return 0;
}

SYS_INIT(atmosic_atmx2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
