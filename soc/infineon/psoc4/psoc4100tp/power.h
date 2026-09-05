/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSoC 4100T Plus power management interface.
 */

#ifndef ZEPHYR_SOC_INFINEON_PSOC4_PSOC4100TP_POWER_H_
#define ZEPHYR_SOC_INFINEON_PSOC4_PSOC4100TP_POWER_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/pm/policy.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_WDT_INFINEON_CAT1)
/**
 * @brief Check whether the WDT is being used as the LPM companion.
 *
 * @return true if the watchdog has been set up and is acting as the Deep
 *         Sleep wakeup source, false otherwise.
 */
bool ifx_wdt_pm_active(void);

/**
 * @brief Get the WDT's remaining feed budget, in WDT (ILO) cycles.
 *
 * Used by the LPM companion hooks to clamp the Deep Sleep window so the
 * system wakes before the application's configured watchdog timeout would
 * elapse.
 *
 * @return Remaining budget, in ILO cycles, before the watchdog would expire.
 */
uint64_t ifx_wdt_pm_budget_cycles(void);

/**
 * @brief Resume the WDT after a Deep Sleep LPM companion cycle.
 *
 * Re-arms the watchdog match window and IRQ state, and consumes
 * @p slept_cycles from the remaining feed budget.
 *
 * @param slept_cycles Number of WDT (ILO) cycles spent asleep.
 */
void ifx_wdt_pm_resume(uint64_t slept_cycles);
#endif /* CONFIG_WDT_INFINEON_CAT1 */

#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
/**
 * @brief Block Deep Sleep entry until the matching unlock call.
 *
 * Requests a Zephyr PM-policy lock on PM_STATE_SUSPEND_TO_IDLE so the idle
 * thread cannot let the system enter Deep Sleep again, used once the WDT's
 * remaining feed budget is too low to safely cover another Deep Sleep cycle.
 *
 * The lock is guarded by @p ds_locked so repeated calls before a matching
 * ifx_wdt_pm_unlock_ds() are a no-op and the underlying policy lock is only
 * ever taken once per outstanding lock/unlock pair.
 *
 * @param ds_locked Pointer to the caller-owned lock state flag.
 */
static inline void ifx_wdt_pm_lock_ds(bool *ds_locked)
{
	if (!*ds_locked) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		*ds_locked = true;
	}
}

/**
 * @brief Release the lock taken by ifx_wdt_pm_lock_ds().
 *
 * @param ds_locked Pointer to the caller-owned lock state flag.
 */
static inline void ifx_wdt_pm_unlock_ds(bool *ds_locked)
{
	if (*ds_locked) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		*ds_locked = false;
	}
}
#endif /* CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_INFINEON_PSOC4_PSOC4100TP_POWER_H_ */
