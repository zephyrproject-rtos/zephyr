/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>

#include "osal_priv.h"

void osal_delay_ms(uint32_t ms)
{
	k_msleep(ms);
}

uint64_t osal_uptime_ms(void)
{
	return (uint64_t)k_uptime_get();
}

void osal_busy_wait_us(uint32_t us)
{
	k_busy_wait(us);
}

uint64_t osal_tick_count(void)
{
	return (uint64_t)k_uptime_ticks();
}

uint32_t osal_tick_freq(void)
{
	return CONFIG_SYS_CLOCK_TICKS_PER_SEC;
}

bool osal_in_isr(void)
{
	return k_is_in_isr();
}
