/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr port of the CPC timer (see port/silabs/src/sli_cpc_timer.c). Ticks
 * are kernel ticks.
 */

#include <string.h>

#include <zephyr/kernel.h>

#include "sl_status.h"
#include "sli_cpc_timer.h"

static void sli_cpc_timer_expiry(struct k_timer *timer)
{
	sli_cpc_timer_t *cpc_timer = CONTAINER_OF(timer, sli_cpc_timer_t, timer);

	cpc_timer->running = false;
	if (cpc_timer->callback != NULL) {
		cpc_timer->callback(cpc_timer, cpc_timer->data);
	}
}

sl_status_t sli_cpc_timer_init(sli_cpc_timer_t *timer)
{
	memset(timer, 0, sizeof(*timer));
	k_timer_init(&timer->timer, sli_cpc_timer_expiry, NULL);

	return SL_STATUS_OK;
}

uint32_t sli_cpc_timer_get_timer_frequency(void)
{
	return CONFIG_SYS_CLOCK_TICKS_PER_SEC;
}

uint32_t sli_cpc_timer_ms_to_tick(uint16_t time_ms)
{
	return k_ms_to_ticks_ceil32(time_ms);
}

uint32_t sli_cpc_timer_tick_to_ms(uint32_t tick)
{
	return k_ticks_to_ms_floor32(tick);
}

uint32_t sli_cpc_timer_get_tick_count(void)
{
	return (uint32_t)k_uptime_ticks();
}

uint64_t sli_cpc_timer_get_tick_count64(void)
{
	return (uint64_t)k_uptime_ticks();
}

sl_status_t sli_cpc_timer_start(sli_cpc_timer_t *timer, uint32_t timeout,
				sli_cpc_timer_callback_t callback, void *callback_data)
{
	timer->callback = callback;
	timer->data = callback_data;
	timer->running = true;
	k_timer_start(&timer->timer, K_TICKS(timeout), K_NO_WAIT);

	return SL_STATUS_OK;
}

sl_status_t sli_cpc_timer_start_timer_ms(sli_cpc_timer_t *timer, uint32_t timeout_ms,
					 sli_cpc_timer_callback_t callback, void *callback_data)
{
	return sli_cpc_timer_start(timer, k_ms_to_ticks_ceil32(timeout_ms), callback,
				   callback_data);
}

sl_status_t sli_cpc_timer_restart(sli_cpc_timer_t *timer, uint32_t timeout,
				  sli_cpc_timer_callback_t callback, void *callback_data)
{
	return sli_cpc_timer_start(timer, timeout, callback, callback_data);
}

sl_status_t sli_cpc_timer_stop(sli_cpc_timer_t *timer)
{
	k_timer_stop(&timer->timer);
	timer->running = false;

	return SL_STATUS_OK;
}

bool sli_cpc_timer_is_running(const sli_cpc_timer_t *timer)
{
	return timer->running;
}

void sli_cpc_timer_delay_millisecond(uint16_t delay_ms)
{
	k_msleep(delay_ms);
}
