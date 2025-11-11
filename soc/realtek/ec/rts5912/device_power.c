/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <reg/reg_system.h>
#include "device_power.h"

void before_rts5912_sleep(void)
{
	__disable_irq();
	__set_BASEPRI(0);
	__ISB();
}

void after_rts5912_sleep(void)
{
	__enable_irq();
	__ISB();
}
