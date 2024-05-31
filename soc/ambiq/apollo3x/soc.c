/*
 * Copyright (c) 2023 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <am_mcu_apollo.h>

static int arm_apollo3_init(void)
{

	/* Initialize for low power in the power control block */
	am_hal_pwrctrl_low_power_init();

	/* Disable the RTC. */
	am_hal_rtc_osc_disable();

	return 0;
}

SYS_INIT(arm_apollo3_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
