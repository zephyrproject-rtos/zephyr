/*
 * Copyright (c) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include "pm_policy.h"

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#include "stm32l1xx_ll_rcc.h"
#include "stm32l1xx_ll_rtc.h"

#define SECS_TO_TICKS		CONFIG_SYS_CLOCK_TICKS_PER_SEC

/* PM Policy based on STM32 SoC/Platform residency requirements */
static const unsigned int pm_min_residency[] = {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_1 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_2 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	CONFIG_SYS_PM_MIN_RESIDENCY_SLEEP_3 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
#ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
	CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_1 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_2
	CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_2 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_3
	CONFIG_SYS_PM_MIN_RESIDENCY_DEEP_SLEEP_3 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP_STATES */
};

enum power_states sys_pm_policy_next_state(int32_t ticks)
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
			LL_RTC_DisableWriteProtection(RTC);
			/* Disable wake up timer to modify it */
			LL_RTC_WAKEUP_Disable(RTC);
			/* wait for WUTF flag set before writing register */
			while (LL_RTC_IsActiveFlag_WUTW(RTC) != 1) {
			}
			/* WUT unit is LSI/4 
			LL_RTC_WAKEUP_SetClock(RTC, LL_RTC_WAKEUPCLOCK_DIV_4);*/
			LL_RTC_WAKEUP_SetClock(RTC, LL_RTC_WAKEUPCLOCK_CKSPRE);
			/* timeout is converted in RTC count units
			uint32_t counts = (32000 / 4 * ticks) / CONFIG_SYS_CLOCK_TICKS_PER_SEC; */
			uint32_t counts = ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
			LL_RTC_WAKEUP_SetAutoReload(RTC, (counts + 1));
			/* Reset RTC Internal Wake up flag */
			LL_RTC_EnableIT_WUT(RTC);
			LL_RTC_WAKEUP_Enable(RTC);

			LL_RTC_ClearFlag_WUT(RTC);

			LL_RTC_EnableWriteProtection(RTC);

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
