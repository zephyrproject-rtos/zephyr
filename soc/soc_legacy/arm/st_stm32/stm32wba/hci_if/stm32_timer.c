/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#include "stm32_timer.h"

#define TICKS_PER_MS (CONFIG_SYS_CLOCK_TICKS_PER_SEC / MSEC_PER_SEC)

static struct k_timer timer;

UTIL_TIMER_Status_t UTIL_TIMER_Create(UTIL_TIMER_Object_t *timer_object,
				      uint32_t period, UTIL_TIMER_Mode_t mode,
				      void (*callback)(void *), void *argument)
{
	if (timer_object == NULL) {
		return UTIL_TIMER_INVALID_PARAM;
	}

	timer_object->ReloadValue = period * TICKS_PER_MS;
	timer_object->Mode = mode;
	timer_object->Callback = callback;
	timer_object->argument = argument;
	timer_object->IsRunning = false;
	timer_object->IsReloadStopped = false;
	timer_object->Next = NULL;

	return UTIL_TIMER_OK;
}

UTIL_TIMER_Status_t UTIL_TIMER_Start(UTIL_TIMER_Object_t *timer_object)
{
	if (timer_object == NULL) {
		return UTIL_TIMER_INVALID_PARAM;
	}

	k_timer_user_data_set(&timer, timer_object);
	k_timer_start(&timer, K_TICKS(timer_object->ReloadValue),
					K_TICKS(timer_object->ReloadValue));
	timer_object->IsRunning = true;

	return UTIL_TIMER_OK;
}

UTIL_TIMER_Status_t UTIL_TIMER_Stop(UTIL_TIMER_Object_t *timer_object)
{
	if (timer_object == NULL) {
		return UTIL_TIMER_INVALID_PARAM;
	}

	k_timer_stop(&timer);
	timer_object->IsRunning = false;

	return UTIL_TIMER_OK;
}
