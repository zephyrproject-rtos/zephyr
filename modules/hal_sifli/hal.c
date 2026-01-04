/*
 * Copyright 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>

/* System clock (48MHz by RC48 at boot) */
uint32_t SystemCoreClock = 48000000UL;

void HAL_Delay_us(uint32_t us)
{
	k_sleep(K_USEC(us));
}
