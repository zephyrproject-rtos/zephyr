/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <timer.h>

static uint32_t saved_time;

static void timer_callback(struct k_timer *_timer)
{
	ARG_UNUSED(_timer);

	TimerIrqHandler();
}

K_TIMER_DEFINE(lora_timer, timer_callback, NULL);

uint32_t RtcGetTimerValue(void)
{
	return k_uptime_get_32();
}

uint32_t RtcGetTimerElapsedTime(void)
{
	return (k_uptime_get_32() - saved_time);
}

uint32_t RtcGetMinimumTimeout(void)
{
	return 1;
}

void RtcStopAlarm(void)
{
	k_timer_stop(&lora_timer);
}

void RtcSetAlarm(uint32_t timeout)
{
	k_timer_start(&lora_timer, K_MSEC(timeout), K_NO_WAIT);
}

uint32_t RtcSetTimerContext(void)
{
	saved_time = k_uptime_get_32();

	return saved_time;
}

/* For us, 1 tick = 1 milli second. So no need to do any conversion here */
uint32_t RtcGetTimerContext(void)
{
	return saved_time;
}

void DelayMsMcu(uint32_t ms)
{
	k_sleep(K_MSEC(ms));
}

uint32_t RtcMs2Tick(uint32_t milliseconds)
{
	return milliseconds;
}

uint32_t RtcTick2Ms(uint32_t tick)
{
	return tick;
}

void BoardCriticalSectionBegin(uint32_t *mask)
{
	*mask = irq_lock();
}

void BoardCriticalSectionEnd(uint32_t *mask)
{
	irq_unlock(*mask);
}
