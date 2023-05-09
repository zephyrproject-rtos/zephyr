/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <am_mcu_apollo.h>

static int arm_apollo4_init(void)
{
	am_hal_pwrctrl_low_power_init();
	am_hal_rtc_osc_disable();

	return 0;
}

SYS_INIT(arm_apollo4_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
