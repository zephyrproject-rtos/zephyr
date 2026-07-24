/*
 * Copyright (c) 2026 Rithic Chellaram Hariharan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Portable light/deep system sleep helpers built on pm_state_force(). */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(pm, CONFIG_PM_LOG_LEVEL);

/* Context-retaining states: everything except ACTIVE and SOFT_OFF. */
#define PM_SLEEP_RETAIN_MIN PM_STATE_RUNTIME_IDLE
#define PM_SLEEP_RETAIN_MAX PM_STATE_SUSPEND_TO_DISK

/* Prefer @p a over @p b for the requested direction, ranking by state depth
 * then by residency for substates of the same state.
 */
static bool pm_sleep_prefer(const struct pm_state_info *a, const struct pm_state_info *b,
			    bool deepest)
{
	if (b == NULL) {
		return true;
	}

	if (a->state != b->state) {
		return deepest ? (a->state > b->state) : (a->state < b->state);
	}

	if (a->min_residency_us != b->min_residency_us) {
		return deepest ? (a->min_residency_us > b->min_residency_us)
			       : (a->min_residency_us < b->min_residency_us);
	}

	return false;
}

/* Pick the deepest or shallowest state in the inclusive [min, max] range. */
static const struct pm_state_info *pm_sleep_pick_state(uint8_t cpu, enum pm_state min,
						       enum pm_state max, bool deepest)
{
	const struct pm_state_info *states;
	const struct pm_state_info *best = NULL;
	uint8_t count;

	/* Enabled states. */
	count = pm_state_cpu_get_all(cpu, &states);
	for (uint8_t i = 0; i < count; i++) {
		if ((states[i].state < min) || (states[i].state > max)) {
			continue;
		}

		if (pm_sleep_prefer(&states[i], best, deepest)) {
			best = &states[i];
		}
	}

	/* Disabled (forceable-only) states, not returned above. */
	for (int s = (int)min; s <= (int)max; s++) {
		const struct pm_state_info *info = pm_state_get(cpu, (enum pm_state)s, 0);

		if ((info != NULL) && pm_sleep_prefer(info, best, deepest)) {
			best = info;
		}
	}

	return best;
}

static int pm_sleep_enter(enum pm_state min, enum pm_state max, bool deepest, k_timeout_t timeout)
{
	uint8_t cpu = CPU_ID;
	const struct pm_state_info *info;

	info = pm_sleep_pick_state(cpu, min, max, deepest);
	if (info == NULL) {
		return -ENOTSUP;
	}

	LOG_DBG("CPU %u entering %s (substate %u)", cpu, pm_state_to_str(info->state),
		info->substate_id);

	if (!pm_state_force(cpu, info)) {
		return -ENOTSUP;
	}

	/* The idle thread enters the forced state for the upcoming idle period
	 * while this thread sleeps. pm_state_force() is consumed on that first
	 * idle entry, so an early wake-up returns to the active PM policy.
	 */
	(void)k_sleep(timeout);

	return 0;
}

int pm_light_sleep(k_timeout_t timeout)
{
	return pm_sleep_enter(PM_SLEEP_RETAIN_MIN, PM_SLEEP_RETAIN_MAX, false, timeout);
}

int pm_deep_sleep(k_timeout_t timeout)
{
	return pm_sleep_enter(PM_SLEEP_RETAIN_MIN, PM_SLEEP_RETAIN_MAX, true, timeout);
}

int pm_soft_off(k_timeout_t timeout)
{
	return pm_sleep_enter(PM_STATE_SOFT_OFF, PM_STATE_SOFT_OFF, true, timeout);
}
