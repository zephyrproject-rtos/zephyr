/*
 * Copyright (c) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include "pm_policy.h"

#include <stm32l4xx_ll_lptim.h>

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#ifdef CONFIG_STM32_LPTIM_CLOCK_LSE
#define LPTIM_CLOCK 32768
#define LPTIM_TIMEBASE 0xFFFF
#else
#define LPTIM_CLOCK 32000
#define LPTIM_TIMEBASE 0xF9FF
#endif

#define LPTIM_MAX_MS (((LPTIM_TIMEBASE + 1) / LPTIM_CLOCK) * 1000)

#define SECS_TO_TICKS		CONFIG_SYS_CLOCK_TICKS_PER_SEC
/* Wakeup delay from stop mode in microseconds */
#define WAKEDELAYSTOP    24
/* Wakeup delay from standby in microseconds */
#define WAKEDELAYSTANDBY    32
/* Wakeup delay from shutdown in microseconds */
#define WAKEDELAYSHUTDOWN   330

u32_t table_prescaler[8] = {
	LL_LPTIM_PRESCALER_DIV1,
	LL_LPTIM_PRESCALER_DIV2,
	LL_LPTIM_PRESCALER_DIV4,
	LL_LPTIM_PRESCALER_DIV8,
	LL_LPTIM_PRESCALER_DIV16,
	LL_LPTIM_PRESCALER_DIV32,
	LL_LPTIM_PRESCALER_DIV64,
	LL_LPTIM_PRESCALER_DIV128,
};

/* PM Policy based on STM32 SoC/Platform residency requirements */
static const unsigned int pm_min_residency[] = {
	CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1 * SECS_TO_TICKS / MSEC_PER_SEC,
	CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_2 * SECS_TO_TICKS / MSEC_PER_SEC,
	CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_3 * SECS_TO_TICKS / MSEC_PER_SEC
};

/* wake up delay in us for each PM_MIN residency timing */
static int wakeupdelay[] = {
	WAKEDELAYSTOP,
	WAKEDELAYSTOP,
	WAKEDELAYSTOP,
	WAKEDELAYSTANDBY,
	WAKEDELAYSHUTDOWN,
};

enum power_states sys_pm_policy_next_state(s32_t ticks)
{
	int i;

	if ((ticks != K_TICKS_FOREVER) && (ticks < pm_min_residency[0])) {
		LOG_DBG("Not enough time for PM operations: %d", ticks);
		return SYS_POWER_STATE_ACTIVE;
	}

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
#ifdef CONFIG_SYS_PM_STATE_LOCK
		if (!sys_pm_ctrl_is_state_enabled((enum power_states)(i))) {
			continue;
		}
#endif
		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= pm_min_residency[i])) {
			/* Set timeout for wakeup event */
			if (ticks != K_TICKS_FOREVER) {
					/* NOTE: Ideally we'd like to set a
					 * timer to wake up just a little
					 * earlier to take care of the wakeup
					 * sequence, ie. by WAKEDELAY
					 * microsecs. However, given
					 * k_timer_start (called later by
					 * ClockP_start) does not currently
					 * have sub-millisecond accuracy, wakeup
					 * would be at up to (WAKEDELAY
					 * + 1 ms) ahead of the next timeout.
					 * This also has the implication that
					 * SYS_PM_MIN_RESIDENCY_SLEEP_2
					 * must be greater than 1.
					 */
				ticks -= (wakeupdelay[i] *
						CONFIG_SYS_CLOCK_TICKS_PER_SEC
						+ 1000000) / 1000000;
				/* now get the corresponding timeout in ms */
				u32_t timeout = ticks * 1000 /
					CONFIG_SYS_CLOCK_TICKS_PER_SEC;
				/* the clock prescaler is set to DIV8 */
				int arr = (((timeout * LPTIM_CLOCK / 8))
					/ 1000) - 1;
				/*
				 * LPTIM setTimeout cannot handle any
				 * more ticks than LPTIM_TIMEBASE
				 */
				arr = MIN(arr, LPTIM_TIMEBASE);
				LL_LPTIM_SetAutoReload(LPTIM1, arr);
				/* ARROK bit validates the write operation */
				LL_LPTIM_ClearFlag_ARROK(LPTIM1);
				/* reset the current counter match value */
				LL_LPTIM_ClearFLAG_ARRM(LPTIM1);
				/* enable and run the LPTIM counter */
				LL_LPTIM_StartCounter(LPTIM1,
					LL_LPTIM_OPERATING_MODE_ONESHOT);
				}

			LOG_DBG("Selected power state %d "
					"(ticks: %d, min_residency: %u)",
					i, ticks, pm_min_residency[i]);
			return (enum power_states)(i);
		}
	}

	LOG_DBG("No suitable power state found!");
	return SYS_POWER_STATE_ACTIVE;
}

__weak bool sys_pm_policy_low_power_devices(enum power_states pm_state)
{
	return sys_pm_is_sleep_state(pm_state);
}
