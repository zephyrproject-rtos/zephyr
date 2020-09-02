/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <sys/__assert.h>
#include "pm_policy.h"

#include <ti/devices/cc13x2_cc26x2/driverlib/sys_ctrl.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SwiP.h>

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#define SECS_TO_TICKS		CONFIG_SYS_CLOCK_TICKS_PER_SEC

/* Wakeup delay from standby in microseconds */
#define WAKEDELAYSTANDBY    240

extern PowerCC26X2_ModuleState PowerCC26X2_module;

/* PM Policy based on SoC/Platform residency requirements */
static const unsigned int pm_min_residency[] = {
#ifdef CONFIG_PM_SLEEP_STATES
	CONFIG_PM_MIN_RESIDENCY_SLEEP_1 * SECS_TO_TICKS / MSEC_PER_SEC,
	CONFIG_PM_MIN_RESIDENCY_SLEEP_2 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif /* CONFIG_PM_SLEEP_STATES */
};

enum power_states pm_policy_next_state(int32_t ticks)
{
	uint32_t constraints;
	bool disallowed = false;
	int i;

	/* check operating conditions, optimally choose DCDC versus GLDO */
	SysCtrl_DCDC_VoltageConditionalControl();

	/* query the declared constraints */
	constraints = Power_getConstraintMask();

	if ((ticks != K_TICKS_FOREVER) && (ticks < pm_min_residency[0])) {
		LOG_DBG("Not enough time for PM operations: %d", ticks);
		return POWER_STATE_ACTIVE;
	}

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
#ifdef CONFIG_PM_STATE_LOCK
		if (!pm_ctrl_is_state_enabled((enum power_states)(i))) {
			continue;
		}
#endif
		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= pm_min_residency[i])) {
			/* Verify if Power module has constraints set to
			 * disallow a state
			 */
			switch (i) {
			case 0: /* Idle mode */
				if ((constraints & (1 <<
				    PowerCC26XX_DISALLOW_IDLE)) != 0) {
					disallowed = true;
				}
				break;
			case 1: /* Standby mode */
				if ((constraints & (1 <<
				    PowerCC26XX_DISALLOW_STANDBY)) != 0) {
					disallowed = true;
				}
				/* Set timeout for wakeup event */
				__ASSERT(
				    CONFIG_PM_MIN_RESIDENCY_SLEEP_2 > 1,
				    "CONFIG_PM_MIN_RESIDENCY_SLEEP_2 must "
				    "be greater than 1.");
				if (ticks != K_TICKS_FOREVER) {
					/* NOTE: Ideally we'd like to set a
					 * timer to wake up just a little
					 * earlier to take care of the wakeup
					 * sequence, ie. by WAKEDELAYSTANDBY
					 * microsecs. However, given
					 * k_timer_start (called later by
					 * ClockP_start) does not currently
					 * have sub-millisecond accuracy, wakeup
					 * would be at up to (WAKEDELAYSTANDBY
					 * + 1 ms) ahead of the next timeout.
					 * This also has the implication that
					 * PM_MIN_RESIDENCY_SLEEP_2
					 * must be greater than 1.
					 */
					ticks -= (WAKEDELAYSTANDBY *
						CONFIG_SYS_CLOCK_TICKS_PER_SEC
						+ 1000000) / 1000000;
#if (CONFIG_SYS_CLOCK_TICKS_PER_SEC <= 1000)
					/*
					 * ClockP_setTimeout cannot handle any
					 * more ticks
					 */
					ticks = MIN(ticks, UINT32_MAX / 1000 *
						CONFIG_SYS_CLOCK_TICKS_PER_SEC);
#endif
					ClockP_setTimeout(ClockP_handle(
						(ClockP_Struct *)
						&PowerCC26X2_module.clockObj),
						ticks);
				}
				break;
			default:
				/* This should never be reached */
				LOG_ERR("Invalid sleep state detected\n");
			}

			if (disallowed) {
				disallowed = false;
				continue;
			}

			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u)",
				i, ticks, pm_min_residency[i]);
			return (enum power_states)(i);
		}
	}

	LOG_DBG("No suitable power state found!");
	return POWER_STATE_ACTIVE;
}

__weak bool pm_policy_low_power_devices(enum power_states pm_state)
{
#ifdef CONFIG_PM_SLEEP_STATES
	return (pm_state == POWER_STATE_SLEEP_2);
#else
	return false;
#endif
}
